#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  mPosition pgPos;
  mSafety pgSafety;
  map<int, mWallet> pgWallet;
  map<double, mTradeDehydrated> pgBuys;
  map<double, mTradeDehydrated> pgSells;
  vector<mProfit> pgProfit;
  double pgTargetBasePos = 0;
  string pgSideAPR = "";
  string pgSideAPR_ = "!=";
  class PG {
    public:
      static void main() {
        load();
        ev_gwDataWallet = [](mWallet k) {
          calcWallet(k);
        };
        ev_ogOrder = [](mOrder k) {
          calcWalletAfterOrder(k);
        };
        ev_mgTargetPosition = []() {
          calcTargetBasePos();
        };
        UI::uiSnap(uiTXT::Position, &onSnapPos);
        UI::uiSnap(uiTXT::TradeSafetyValue, &onSnapSafety);
        UI::uiSnap(uiTXT::TargetBasePosition, &onSnapTargetBasePos);
      };
      static void calcSafety() {
        if (!pgPos.value or !mgFairValue) return;
        mSafety safety = nextSafety();
        if (pgSafety.buyPing == -1
          or abs(safety.combined - pgSafety.combined) > 1e-3
          or abs(safety.buyPing - pgSafety.buyPing) >= 1e-2
          or abs(safety.sellPong - pgSafety.sellPong) >= 1e-2
        ) {
          pgSafety = safety;
          UI::uiSend(uiTXT::TradeSafetyValue, pgSafety, true);
        }
      };
      static void calcTargetBasePos() {
        if (!pgPos.value) { cout << FN::uiT() << "QE" << RRED << " Warrrrning:" << BRED << " Unable to calculate TBP, missing market data." << endl; return; }
        double targetBasePosition = ((mAutoPositionMode)QP::getInt("autoPositionMode") == mAutoPositionMode::Manual)
          ? (QP::getBool("percentageValues")
            ? QP::getDouble("targetBasePositionPercentage") * pgPos.value / 1e+2
            : QP::getDouble("targetBasePosition"))
          : ((1 + mgTargetPos) / 2) * pgPos.value;
        if (pgTargetBasePos and abs(pgTargetBasePos - targetBasePosition) < 1e-4 and pgSideAPR_ == pgSideAPR) return;
        pgTargetBasePos = targetBasePosition;
        pgSideAPR_ = pgSideAPR;
        ev_pgTargetBasePosition();
        json k = {{"tbp", pgTargetBasePos}, {"sideAPR", pgSideAPR}};
        UI::uiSend(uiTXT::TargetBasePosition, k, true);
        DB::insert(uiTXT::TargetBasePosition, k);
        cout << FN::uiT() << "TBP " << (int)(pgTargetBasePos / pgPos.value * 1e+2) << "% = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << endl;
      };
      static void addTrade(mTradeHydrated k) {
        mTradeDehydrated k_(k.price, k.quantity, k.time);
        if (k.side == mSide::Bid) pgBuys[k.price] = k_;
        else pgSells[k.price] = k_;
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::TargetBasePosition);
        if (k.size()) {
          k = k.at(0);
          pgTargetBasePos = k.value("tbp", 0.0);
          pgSideAPR = k.value("sideAPR", "");
        }
        cout << FN::uiT() << "DB" << RWHITE << " loaded TBP = " << setprecision(8) << fixed << pgTargetBasePos << " " << mCurrency[gw->base] << "." << endl;
      };
      static json onSnapPos() {
        return { pgPos };
      };
      static json onSnapSafety() {
        return { pgSafety };
      };
      static json onSnapTargetBasePos() {
        return {{{"tbp", pgTargetBasePos}, {"sideAPR", pgSideAPR}}};
      };
      static mSafety nextSafety() {
        double buySize = QP::getBool("percentageValues")
          ? QP::getDouble("buySizePercentage") * pgPos.value / 100
          : QP::getDouble("buySize");
        double sellSize = QP::getBool("percentageValues")
          ? QP::getDouble("sellSizePercentage") * pgPos.value / 100
          : QP::getDouble("sellSize");
        double totalBasePosition = pgPos.baseAmount + pgPos.baseHeldAmount;
        if (QP::getBool("buySizeMax") and (mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off)
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if (QP::getBool("sellSizeMax") and (mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off)
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        double widthPong = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPongPercentage") * mgFairValue / 100
          : QP::getDouble("widthPong");
        map<double, mTradeHydrated> tradesBuy;
        map<double, mTradeHydrated> tradesSell;
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          if (it->side == mSide::Bid)
            tradesBuy[it->price] = *it;
          else tradesSell[it->price] = *it;
        double buyPing = 0;
        double sellPong = 0;
        double buyQty = 0;
        double sellQty = 0;
        if ((mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingFair
          or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
        ) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong*-1);
        } else if ((mPongAt)QP::getInt("pongAt") == mPongAt::LongPingFair
          or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
        ) {
          matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchLastPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong, true);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPong /= sellQty;
        clean();
        double sumBuys = sum(&pgBuys);
        double sumSells = sum(&pgSells);
        return mSafety(
          sumBuys / buySize,
          sumSells / sellSize,
          (sumBuys + sumSells) / (buySize + sellSize),
          buyPing,
          sellPong
        );
      };
      static void matchFirstPing(map<double, mTradeHydrated>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchBestPing(map<double, mTradeHydrated>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchLastPing(map<double, mTradeHydrated>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(QP::matchPings(), false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      static void matchPing(bool matchPings, bool near, bool far, map<double, mTradeHydrated>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<double, mTradeHydrated>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<double, mTradeHydrated>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * mgFairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      static bool matchPing(bool matchPings, bool near, bool far, double *ping, double width, double* qty, double qtyMax, double fv, double price, double qtyTrade, double priceTrade, double KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (*qty < qtyMax
          and (far ? fv > price : true)
          and (near ? (reverse ? fv - width : fv + width) < price : true)
          and (!matchPings or KqtyTrade < qtyTrade)
        ) matchPing(ping, qty, qtyMax, qtyTrade, priceTrade);
        return *qty >= qtyMax;
      };
      static void matchPing(double* ping, double* qty, double qtyMax, double qtyTrade, double priceTrade) {
        double qty_ = fmin(qtyMax - *qty, qtyTrade);
        *ping += priceTrade * qty_;
        *qty += qty_;
      };
      static void clean() {
        if (pgBuys.size()) expire(&pgBuys);
        if (pgSells.size()) expire(&pgSells);
        skip();
      };
      static void expire(map<double, mTradeDehydrated>* k) {
        unsigned long now = FN::T();
        for (map<double, mTradeDehydrated>::iterator it = k->begin(); it != k->end();)
          if (it->second.time + QP::getDouble("tradeRateSeconds") * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      static void skip() {
        while (pgBuys.size() and pgSells.size()) {
          mTradeDehydrated buy = pgBuys.rbegin()->second;
          mTradeDehydrated sell = pgSells.begin()->second;
          if (sell.price < buy.price) break;
          double buyQty = buy.quantity;
          buy.quantity = buyQty - sell.quantity;
          sell.quantity = sell.quantity - buyQty;
          if (buy.quantity < gw->minSize)
            pgBuys.erase(--pgBuys.rbegin().base());
          if (sell.quantity < gw->minSize)
            pgSells.erase(pgSells.begin());
        }
      };
      static double sum(map<double, mTradeDehydrated>* k) {
        double sum = 0;
        for (map<double, mTradeDehydrated>::iterator it = k->begin(); it != k->end(); ++it)
          sum += it->second.quantity;
        return sum;
      };
      static void calcWallet(mWallet k) {
        if (k.currency>-1)  pgWallet[k.currency] = k;
        if (!mgFairValue or pgWallet.find(gw->base) == pgWallet.end() or pgWallet.find(gw->quote) == pgWallet.end()) return;
        double baseAmount = pgWallet[gw->base].amount;
        double quoteAmount = pgWallet[gw->quote].amount;
        double baseValue = baseAmount + quoteAmount / mgFairValue + pgWallet[gw->base].held + pgWallet[gw->quote].held / mgFairValue;
        double quoteValue = baseAmount * mgFairValue + quoteAmount + pgWallet[gw->base].held * mgFairValue + pgWallet[gw->quote].held;
        unsigned long now = FN::T();
        pgProfit.push_back(mProfit(baseValue, quoteValue, now));
        for (vector<mProfit>::iterator it = pgProfit.begin(); it != pgProfit.end();)
          if (it->time + (QP::getDouble("profitHourInterval") * 36e+5) > now) ++it;
          else it = pgProfit.erase(it);
        mPosition pos(
          baseAmount,
          quoteAmount,
          pgWallet[gw->base].held,
          pgWallet[gw->quote].held,
          baseValue,
          quoteValue,
          ((baseValue - pgProfit.begin()->baseValue) / baseValue) * 1e+2,
          ((quoteValue - pgProfit.begin()->quoteValue) / quoteValue) * 1e+2,
          mPair(gw->base, gw->quote),
          gw->exchange
        );
        bool eq = true;
        if (pgPos.value) {
          eq = abs(pos.value - pgPos.value) < 2e-6;
          if(eq
            and abs(pos.quoteValue - pgPos.quoteValue) < 2e-2
            and abs(pos.baseAmount - pgPos.baseAmount) < 2e-6
            and abs(pos.quoteAmount - pgPos.quoteAmount) < 2e-2
            and abs(pos.baseHeldAmount - pgPos.baseHeldAmount) < 2e-6
            and abs(pos.quoteHeldAmount - pgPos.quoteHeldAmount) < 2e-2
            and abs(pos.profitBase - pgPos.profitBase) < 2e-2
            and abs(pos.profitQuote - pgPos.profitQuote) < 2e-2
          ) return;
        }
        pgPos = pos;
        if (!eq) calcTargetBasePos();
        UI::uiSend(uiTXT::Position, pgPos, true);
      };
      static void calcWalletAfterOrder(mOrder k) {
        if (!pgPos.value) return;
        double heldAmount = 0;
        double amount = k.side == mSide::Ask
          ? pgPos.baseAmount + pgPos.baseHeldAmount
          : pgPos.quoteAmount + pgPos.quoteHeldAmount;
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (it->second.side != k.side) return;
          double held = it->second.quantity * (it->second.side == mSide::Bid ? it->second.price : 1);
          if (amount >= held) {
            amount -= held;
            heldAmount += held;
          }
        }
        calcWallet(mWallet(amount, heldAmount, k.side == mSide::Ask
            ? k.pair.base : k.pair.quote
        ));
      };
  };
}

#endif
