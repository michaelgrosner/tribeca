#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass {
    private:
      vector<mProfit> profits;
      map<double, mTrade> buys;
      map<double, mTrade> sells;
      map<string, mWallet> balance;
      unsigned long profitT_21s = 0;
      string sideAPR_ = "!=";
    public:
      mPosition position;
      mSafety safety;
      double targetBasePosition = 0;
      double positionDivergence = 0;
      string sideAPR = "";
    protected:
      void load() {
        for (json &it : ((DB*)memory)->load(mMatter::Position))
          profits.push_back(it);
        FN::log("DB", string("loaded ") + to_string(profits.size()) + " historical Profits");
        json k = ((DB*)memory)->load(mMatter::TargetBasePosition);
        if (!k.empty()) {
          k = k.at(0);
          targetBasePosition = k.value("tbp", 0.0);
          if (k.find("pDiv") != k.end()) positionDivergence = k.value("pDiv", 0.0);
          sideAPR = k.value("sideAPR", "");
        }
        stringstream ss;
        ss << setprecision(8) << fixed << targetBasePosition;
        FN::log("DB", string("loaded TBP = ") + ss.str() + " " + gw->base);
      };
      void waitData() {
        gw->evDataWallet = [&](mWallet k) { _debugEvent_
          calcWallet(k);
        };
        ((EV*)events)->ogOrder = [&](mOrder k) { _debugEvent_
          calcWalletAfterOrder(k);
          FN::screen_refresh(((OG*)broker)->orders);
        };
        ((EV*)events)->mgTargetPosition = [&]() { _debugEvent_
          calcTargetBasePos();
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(mMatter::Position, &helloPosition);
        ((UI*)client)->welcome(mMatter::TradeSafetyValue, &helloSafety);
        ((UI*)client)->welcome(mMatter::TargetBasePosition, &helloTargetBasePos);
      };
    public:
      void calcSafety() {
        if (position.empty() or !((MG*)market)->fairValue) return;
        mSafety next = nextSafety();
        if (safety.buyPing == -1
          or next.combined != safety.combined
          or next.buyPing != safety.buyPing
          or next.sellPing != safety.sellPing
        ) {
          safety = next;
          ((UI*)client)->send(mMatter::TradeSafetyValue, next);
        }
      };
      void calcSafetyAfterTrade(mTrade k) {
        (k.side == mSide::Bid
          ? buys : sells
        )[k.price] = mTrade(
          k.price,
          k.quantity,
          k.time
        );
        calcSafety();
      };
      void calcTargetBasePos() {
        if (position.empty()) return FN::logWar("PG", "Unable to calculate TBP, missing wallet data");
        double baseValue = position.baseValue;
        double next = qp->autoPositionMode == mAutoPositionMode::Manual
          ? (qp->percentageValues
            ? qp->targetBasePositionPercentage * baseValue / 1e+2
            : qp->targetBasePosition)
          : ((1 + ((MG*)market)->targetPosition) / 2) * baseValue;
        if (targetBasePosition and abs(targetBasePosition - next) < 1e-4 and sideAPR_ == sideAPR) return;
        targetBasePosition = next;
        sideAPR_ = sideAPR;
        calcPDiv(baseValue);
        ((EV*)events)->pgTargetBasePosition();
        json k = {{"tbp", targetBasePosition}, {"sideAPR", sideAPR}, {"pDiv", positionDivergence }};
        ((UI*)client)->send(mMatter::TargetBasePosition, k);
        ((DB*)memory)->insert(mMatter::TargetBasePosition, k);
        stringstream ss;
        ss << (int)(targetBasePosition / baseValue * 1e+2) << "% = " << setprecision(8) << fixed << targetBasePosition;
        stringstream ss_;
        ss_ << (int)(positionDivergence  / baseValue * 1e+2) << "% = " << setprecision(8) << fixed << positionDivergence;
        FN::log("PG", string("TBP: ") + ss.str() + " " + gw->base + ", pDiv: " + ss_.str() + " " + gw->base);
      };
    private:
      function<void(json*)> helloPosition = [&](json *welcome) {
        *welcome = { position };
      };
      function<void(json*)> helloSafety = [&](json *welcome) {
        *welcome = { safety };
      };
      function<void(json*)> helloTargetBasePos = [&](json *welcome) {
        *welcome = { {
          {"tbp", targetBasePosition},
          {"sideAPR", sideAPR},
          {"pDiv", positionDivergence}
        } };
      };
      mSafety nextSafety() {
        double buySize = qp->percentageValues
          ? qp->buySizePercentage * position.baseValue / 100
          : qp->buySize;
        double sellSize = qp->percentageValues
          ? qp->sellSizePercentage * position.baseValue / 100
          : qp->sellSize;
        map<double, mTrade> tradesBuy;
        map<double, mTrade> tradesSell;
        for (mTrade &it: ((OG*)broker)->tradesHistory) {
          (it.side == mSide::Bid ? tradesBuy : tradesSell)[it.price] = it;
          if (qp->safety == mQuotingSafety::PingPong)
            (it.side == mSide::Bid ? buySize : sellSize) = it.quantity;
        }
        double totalBasePosition = position.baseAmount + position.baseHeldAmount;
        if (qp->aggressivePositionRebalancing != mAPR::Off) {
          if (qp->buySizeMax) buySize = fmax(buySize, targetBasePosition - totalBasePosition);
          if (qp->sellSizeMax) sellSize = fmax(sellSize, totalBasePosition - targetBasePosition);
        }
        double widthPong = qp->widthPercentage
          ? qp->widthPongPercentage * ((MG*)market)->fairValue / 100
          : qp->widthPong;
        double buyPing = 0,
               sellPing = 0,
               buyQty = 0,
               sellQty = 0;
        if (qp->pongAt == mPongAt::ShortPingFair or qp->pongAt == mPongAt::ShortPingAggressive) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
        } else if (qp->pongAt == mPongAt::LongPingFair or qp->pongAt == mPongAt::LongPingAggressive) {
          matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPing /= sellQty;
        clean();
        double sumBuys = sum(&buys);
        double sumSells = sum(&sells);
        return mSafety(
          sumBuys / buySize,
          sumSells / sellSize,
          (sumBuys + sumSells) / (buySize + sellSize),
          buyPing,
          sellPing
        );
      };
      void matchFirstPing(map<double, mTrade> *trades, double *ping, double *qty, double qtyMax, double width, bool reverse = false) {
        matchPing(true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<double, mTrade> *trades, double *ping, double *qty, double qtyMax, double width, bool reverse = false) {
        matchPing(true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<double, mTrade> *trades, double *ping, double *qty, double qtyMax, double width, bool reverse = false) {
        matchPing(false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchPing(bool near, bool far, map<double, mTrade> *trades, double *ping, double *qty, double qtyMax, double width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<double, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(near, far, ping, qty, qtyMax, width, dir * ((MG*)market)->fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<double, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(near, far, ping, qty, qtyMax, width, dir * ((MG*)market)->fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      bool matchPing(bool near, bool far, double *ping, double *qty, double qtyMax, double width, double fv, double price, double qtyTrade, double priceTrade, double KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (*qty < qtyMax
          and (far ? fv > price : true)
          and (near ? (reverse ? fv - width : fv + width) < price : true)
          and (!qp->_matchPings or KqtyTrade < qtyTrade)
        ) {
          double qty_ = fmin(qtyMax - *qty, qtyTrade);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax;
      };
      void clean() {
        if (buys.size()) expire(&buys);
        if (sells.size()) expire(&sells);
        skip();
      };
      void expire(map<double, mTrade> *k) {
        unsigned long now = FN::T();
        for (map<double, mTrade>::iterator it = k->begin(); it != k->end();)
          if (it->second.time + qp->tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (buys.size() and sells.size()) {
          mTrade buy = buys.rbegin()->second;
          mTrade sell = sells.begin()->second;
          if (sell.price < buy.price) break;
          double buyQty = buy.quantity;
          buy.quantity = buyQty - sell.quantity;
          sell.quantity = sell.quantity - buyQty;
          if (buy.quantity < gw->minSize)
            buys.erase(--buys.rbegin().base());
          if (sell.quantity < gw->minSize)
            sells.erase(sells.begin());
        }
      };
      double sum(map<double, mTrade> *k) {
        double sum = 0;
        for (map<double, mTrade>::value_type &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void calcWallet(mWallet k) {
        if (k.currency!="") balance[k.currency] = k;
        if (balance.find(gw->quote) == balance.end()) balance[gw->quote] = mWallet(0, 0, gw->quote);
        if (balance.find(gw->base) == balance.end()) balance[gw->base] = mWallet(0, 0, gw->base);
        if (!((MG*)market)->fairValue or balance.find(gw->base) == balance.end() or balance.find(gw->quote) == balance.end()) return;
        mPosition pos(
          balance[gw->base].amount,
          balance[gw->quote].amount,
          balance[gw->base].held,
          balance[gw->quote].held,
          balance[gw->base].amount + balance[gw->quote].amount / ((MG*)market)->fairValue + balance[gw->base].held + balance[gw->quote].held / ((MG*)market)->fairValue,
          balance[gw->base].amount * ((MG*)market)->fairValue + balance[gw->quote].amount + balance[gw->base].held * ((MG*)market)->fairValue + balance[gw->quote].held,
          position.profitBase,
          position.profitQuote,
          mPair(gw->base, gw->quote)
        );
        calcProfit(&pos);
        bool eq = true;
        if (!position.empty()) {
          eq = abs(pos.baseValue - position.baseValue) < 2e-6;
          if(eq
            and abs(pos.quoteValue - position.quoteValue) < 2e-2
            and abs(pos.baseAmount - position.baseAmount) < 2e-6
            and abs(pos.quoteAmount - position.quoteAmount) < 2e-2
            and abs(pos.baseHeldAmount - position.baseHeldAmount) < 2e-6
            and abs(pos.quoteHeldAmount - position.quoteHeldAmount) < 2e-2
            and abs(pos.profitBase - position.profitBase) < 2e-2
            and abs(pos.profitQuote - position.profitQuote) < 2e-2
          ) return;
        }
        position = pos;
        if (!eq) calcTargetBasePos();
        ((UI*)client)->send(mMatter::Position, pos);
      };
      void calcWalletAfterOrder(mOrder k) {
        if (position.empty()) return;
        double heldAmount = 0;
        double amount = k.side == mSide::Ask
          ? position.baseAmount + position.baseHeldAmount
          : position.quoteAmount + position.quoteHeldAmount;
        for (map<string, mOrder>::value_type &it : ((OG*)broker)->orders)
          if (it.second.side == k.side) {
            double held = it.second.quantity * (it.second.side == mSide::Bid ? it.second.price : 1);
            if (amount >= held) {
              amount -= held;
              heldAmount += held;
            }
          }
        double completedAmount = k.tradeQuantity * (k.side == mSide::Ask ? k.price : 1);
        if (completedAmount) {
          double oppositeAmount = k.side == mSide::Ask
            ? position.quoteAmount
            : position.baseAmount;
          double oppositeHeldAmount = k.side == mSide::Ask
            ? position.quoteHeldAmount
            : position.baseHeldAmount;
          calcWallet(mWallet(fmax(0, oppositeAmount + completedAmount), oppositeHeldAmount, k.side == mSide::Ask ? k.pair.quote : k.pair.base));
        }
        completedAmount = k.tradeQuantity * (k.side == mSide::Bid ? k.price : 1);
        calcWallet(mWallet(fmax(0, amount - completedAmount), heldAmount, k.side == mSide::Ask ? k.pair.base : k.pair.quote));
      };
      void calcPDiv(double baseValue) {
        double pDiv = qp->percentageValues
            ? qp->positionDivergencePercentage * baseValue / 1e+2
            : qp->positionDivergence;
        if (qp->autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp->positionDivergenceMode) positionDivergence = pDiv;
        else {
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          double pDivMin = qp->percentageValues
            ? qp->positionDivergencePercentageMin * baseValue / 1e+2
            : qp->positionDivergenceMin;
          if (mPDivMode::Linear == qp->positionDivergenceMode) positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (mPDivMode::Sine == qp->positionDivergenceMode) positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (mPDivMode::SQRT == qp->positionDivergenceMode) positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (mPDivMode::Switch == qp->positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
      }
      void calcProfit(mPosition *k) {
        unsigned long now = FN::T();
        if (profitT_21s<=3) ++profitT_21s;
        else if (k->baseValue and k->quoteValue and profitT_21s+21e+3 < now) {
          profitT_21s = now;
          mProfit profit(k->baseValue, k->quoteValue, now);
          ((DB*)memory)->insert(mMatter::Position, profit, false, "NULL", now - (qp->profitHourInterval * 36e+5));
          profits.push_back(profit);
          for (vector<mProfit>::iterator it = profits.begin(); it != profits.end();)
            if (it->time + (qp->profitHourInterval * 36e+5) > now) ++it;
            else it = profits.erase(it);
          k->profitBase = ((k->baseValue - profits.begin()->baseValue) / k->baseValue) * 1e+2;
          k->profitQuote = ((k->quoteValue - profits.begin()->quoteValue) / k->quoteValue) * 1e+2;
        }
      }
  };
}

#endif
