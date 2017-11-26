#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass {
    private:
      vector<mProfit> profits;
      map<double, mTrade> buys;
      map<double, mTrade> sells;
      map<string, mWallet> balance;
      mutex /*profitMutex,
            */balanceMutex;
    public:
      mPosition position;
      mSafety safety;
      double targetBasePosition = 0;
      double positionDivergence = 0;
      string sideAPR = "";
      mutex pgMutex;
    protected:
      void load() {
        json k = ((DB*)memory)->load(uiTXT::TargetBasePosition);
        if (k.size()) {
          k = k.at(0);
          targetBasePosition = k.value("tbp", 0.0);
          sideAPR = k.value("sideAPR", "");
        }
        stringstream ss;
        ss << setprecision(8) << fixed << targetBasePosition;
        FN::log("DB", string("loaded TBP = ") + ss.str() + " " + gw->base);
        k = ((DB*)memory)->load(uiTXT::Position);
        if (k.size()) {
          for (json::reverse_iterator it = k.rbegin(); it != k.rend(); ++it)
            profits.push_back(mProfit(
              (*it)["baseValue"].get<double>(),
              (*it)["quoteValue"].get<double>(),
              (*it)["time"].get<unsigned long>()
            ));
          FN::log("DB", string("loaded ") + to_string(profits.size()) + " historical Profits");
        }
        balance[gw->base] = mWallet(0, 0, gw->base);
        balance[gw->quote] = mWallet(0, 0, gw->quote);
      };
      void waitData() {
        gw->evDataWallet = [&](mWallet k) {
          ((EV*)events)->debug(string("PG evDataWallet mWallet ") + ((json)k).dump());
          calcWallet(k);
        };
        ((EV*)events)->ogOrder = [&](mOrder k) {
          ((EV*)events)->debug(string("PG ogOrder mOrder ") + ((json)k).dump());
          calcWalletAfterOrder(k);
          FN::screen_refresh(((OG*)broker)->ordersBothSides());
        };
        ((EV*)events)->mgTargetPosition = [&]() {
          ((EV*)events)->debug("PG mgTargetPosition");
          calcTargetBasePos();
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::Position, &helloPosition);
        ((UI*)client)->welcome(uiTXT::TradeSafetyValue, &helloSafety);
        ((UI*)client)->welcome(uiTXT::TargetBasePosition, &helloTargetBasePos);
      };
    public:
      void calcSafety() {
        if (empty() or !((MG*)market)->fairValue) return;
        mSafety next = nextSafety();
        pgMutex.lock();
        if (safety.buyPing == -1
          or next.combined != safety.combined
          or next.buyPing != safety.buyPing
          or next.sellPong != safety.sellPong
        ) {
          safety = next;
          pgMutex.unlock();
          ((UI*)client)->send(uiTXT::TradeSafetyValue, next);
        } else pgMutex.unlock();
      };
      void calcTargetBasePos() {
        static string sideAPR_ = "!=";
        if (empty()) { FN::logWar("QE", "Unable to calculate TBP, missing market data."); return; }
        pgMutex.lock();
        double value = position.value;
        pgMutex.unlock();
        double next = qp->autoPositionMode == mAutoPositionMode::Manual
          ? (qp->percentageValues
            ? qp->targetBasePositionPercentage * value / 1e+2
            : qp->targetBasePosition)
          : ((1 + ((MG*)market)->targetPosition) / 2) * value;
        if (targetBasePosition and abs(targetBasePosition - next) < 1e-4 and sideAPR_ == sideAPR) return;
        targetBasePosition = next;
        sideAPR_ = sideAPR;
        if (qp->autoPositionMode != mAutoPositionMode::Manual) calcDynamicPDiv(value);
        ((EV*)events)->pgTargetBasePosition();
        json k = {{"tbp", targetBasePosition}, {"sideAPR", sideAPR}, {"pDiv", positionDivergence }};
        ((UI*)client)->send(uiTXT::TargetBasePosition, k, true);
        ((DB*)memory)->insert(uiTXT::TargetBasePosition, k);
        stringstream ss;
        ss << (int)(targetBasePosition / value * 1e+2) << "% = " << setprecision(8) << fixed << targetBasePosition;
        stringstream ss_;
        ss_ << (int)(positionDivergence  / value * 1e+2) << "% = " << setprecision(8) << fixed << positionDivergence ;
        FN::log("PG", string("TBP: ") + ss.str() + " " + gw->base + ", pDiv: " + ss_.str() + " " + gw->base);
      };
      void addTrade(mTrade k) {
        mTrade k_(k.price, k.quantity, k.time);
        if (k.side == mSide::Bid) buys[k.price] = k_;
        else sells[k.price] = k_;
      };
      bool empty() {
        lock_guard<mutex> lock(pgMutex);
        return !position.value;
      };
    private:
      function<json()> helloPosition = [&]() {
        lock_guard<mutex> lock(pgMutex);
        return (json){ position };
      };
      function<json()> helloSafety = [&]() {
        lock_guard<mutex> lock(pgMutex);
        return (json){ safety };
      };
      function<json()> helloTargetBasePos = [&]() {
        return (json){{{"tbp", targetBasePosition}, {"sideAPR", sideAPR}, {"pDiv", positionDivergence }}};
      };
      mSafety nextSafety() {
        pgMutex.lock();
        double value          = position.value,
               baseAmount     = position.baseAmount,
               baseHeldAmount = position.baseHeldAmount;
        pgMutex.unlock();
        double buySize = qp->percentageValues
          ? qp->buySizePercentage * value / 100
          : qp->buySize;
        double sellSize = qp->percentageValues
          ? qp->sellSizePercentage * value / 100
          : qp->sellSize;
        double totalBasePosition = baseAmount + baseHeldAmount;
        if (qp->buySizeMax and qp->aggressivePositionRebalancing != mAPR::Off)
          buySize = fmax(buySize, targetBasePosition - totalBasePosition);
        if (qp->sellSizeMax and qp->aggressivePositionRebalancing != mAPR::Off)
          sellSize = fmax(sellSize, totalBasePosition - targetBasePosition);
        double widthPong = qp->widthPercentage
          ? qp->widthPongPercentage * ((MG*)market)->fairValue / 100
          : qp->widthPong;
        map<double, mTrade> tradesBuy;
        map<double, mTrade> tradesSell;
        for (vector<mTrade>::iterator it = ((OG*)broker)->tradesHistory.begin(); it != ((OG*)broker)->tradesHistory.end(); ++it)
          if (it->side == mSide::Bid)
            tradesBuy[it->price] = *it;
          else tradesSell[it->price] = *it;
        double buyPing = 0;
        double sellPong = 0;
        double buyQty = 0;
        double sellQty = 0;
        if (qp->pongAt == mPongAt::ShortPingFair
          or qp->pongAt == mPongAt::ShortPingAggressive
        ) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong*-1);
        } else if (qp->pongAt == mPongAt::LongPingFair
          or qp->pongAt == mPongAt::LongPingAggressive
        ) {
          matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchLastPing(&tradesSell, &sellPong, &sellQty, buySize, widthPong, true);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPong /= sellQty;
        clean();
        double sumBuys = sum(&buys);
        double sumSells = sum(&sells);
        return mSafety(
          sumBuys / buySize,
          sumSells / sellSize,
          (sumBuys + sumSells) / (buySize + sellSize),
          buyPing,
          sellPong
        );
      };
      void matchFirstPing(map<double, mTrade>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(qp->_matchPings, true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<double, mTrade>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(qp->_matchPings, true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<double, mTrade>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        matchPing(qp->_matchPings, false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchPing(bool matchPings, bool near, bool far, map<double, mTrade>* trades, double* ping, double* qty, double qtyMax, double width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<double, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * ((MG*)market)->fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<double, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(matchPings, near, far, ping, width, qty, qtyMax, dir * ((MG*)market)->fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      bool matchPing(bool matchPings, bool near, bool far, double *ping, double width, double* qty, double qtyMax, double fv, double price, double qtyTrade, double priceTrade, double KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (*qty < qtyMax
          and (far ? fv > price : true)
          and (near ? (reverse ? fv - width : fv + width) < price : true)
          and (!matchPings or KqtyTrade < qtyTrade)
        ) matchPing(ping, qty, qtyMax, qtyTrade, priceTrade);
        return *qty >= qtyMax;
      };
      void matchPing(double* ping, double* qty, double qtyMax, double qtyTrade, double priceTrade) {
        double qty_ = fmin(qtyMax - *qty, qtyTrade);
        *ping += priceTrade * qty_;
        *qty += qty_;
      };
      void clean() {
        if (buys.size()) expire(&buys);
        if (sells.size()) expire(&sells);
        skip();
      };
      void expire(map<double, mTrade>* k) {
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
      double sum(map<double, mTrade>* k) {
        double sum = 0;
        for (map<double, mTrade>::iterator it = k->begin(); it != k->end(); ++it)
          sum += it->second.quantity;
        return sum;
      };
      void calcWallet(mWallet k) {
        static unsigned long profitT_21s = 0;
        balanceMutex.lock();
        if (k.currency!="") balance[k.currency] = k;
        if (!((MG*)market)->fairValue or balance.find(gw->base) == balance.end() or balance.find(gw->quote) == balance.end()) {
          balanceMutex.unlock();
          return;
        }
        mWallet baseWallet = balance[gw->base];
        mWallet quoteWallet = balance[gw->quote];
        balanceMutex.unlock();
        double baseValue = baseWallet.amount + quoteWallet.amount / ((MG*)market)->fairValue + baseWallet.held + quoteWallet.held / ((MG*)market)->fairValue;
        double quoteValue = baseWallet.amount * ((MG*)market)->fairValue + quoteWallet.amount + baseWallet.held * ((MG*)market)->fairValue + quoteWallet.held;
        unsigned long now = FN::T();
        mProfit profit(baseValue, quoteValue, now);
        double profitBase = 0;
        double profitQuote = 0;
        if (baseValue and quoteValue) {
          if (profitT_21s+21e+3 < FN::T()) {
            profitT_21s = FN::T();
            ((DB*)memory)->insert(uiTXT::Position, profit, false, "NULL", now - qp->profitHourInterval * 36e+5);
          }
          // profitMutex.lock();
          profits.push_back(profit);
          for (vector<mProfit>::iterator it = profits.begin(); it != profits.end();)
            if (it->time + (qp->profitHourInterval * 36e+5) > now) ++it;
            else it = profits.erase(it);
          profitBase = ((baseValue - profits.begin()->baseValue) / baseValue) * 1e+2;
          profitQuote = ((quoteValue - profits.begin()->quoteValue) / quoteValue) * 1e+2;
          // profitMutex.unlock();
        }
        mPosition pos(
          baseWallet.amount,
          quoteWallet.amount,
          baseWallet.held,
          quoteWallet.held,
          baseValue,
          quoteValue,
          profitBase,
          profitQuote,
          mPair(gw->base, gw->quote),
          gw->exchange
        );
        bool eq = true;
        if (!empty()) {
          pgMutex.lock();
          eq = abs(pos.value - position.value) < 2e-6;
          if(eq
            and abs(pos.quoteValue - position.quoteValue) < 2e-2
            and abs(pos.baseAmount - position.baseAmount) < 2e-6
            and abs(pos.quoteAmount - position.quoteAmount) < 2e-2
            and abs(pos.baseHeldAmount - position.baseHeldAmount) < 2e-6
            and abs(pos.quoteHeldAmount - position.quoteHeldAmount) < 2e-2
            and abs(pos.profitBase - position.profitBase) < 2e-2
            and abs(pos.profitQuote - position.profitQuote) < 2e-2
          ) { pgMutex.unlock(); return; }
        } else pgMutex.lock();
        position = pos;
        pgMutex.unlock();
        if (!eq) calcTargetBasePos();
        ((UI*)client)->send(uiTXT::Position, pos, true);
      };
      void calcWalletAfterOrder(mOrder k) {
        if (empty()) return;
        double heldAmount = 0;
        pgMutex.lock();
        double amount = k.side == mSide::Ask
          ? position.baseAmount + position.baseHeldAmount
          : position.quoteAmount + position.quoteHeldAmount;
        pgMutex.unlock();
        multimap<double, mOrder> ordersSide = ((OG*)broker)->ordersAtSide(k.side);
        for (multimap<double, mOrder>::iterator it = ordersSide.begin(); it != ordersSide.end(); ++it) {
          double held = it->second.quantity * (it->second.side == mSide::Bid ? it->second.price : 1);
          if (amount >= held) {
            amount -= held;
            heldAmount += held;
          }
        }
        calcWallet(mWallet(amount, heldAmount, k.side == mSide::Ask ? k.pair.base : k.pair.quote));
      };
      void calcDynamicPDiv(double value) {
        double divCenter = 1 - abs((targetBasePosition / value * 2) - 1);
        double pDiv = qp->percentageValues
            ? qp->positionDivergencePercentage * value / 1e+2
            : qp->positionDivergence;
          double pDivMin = qp->percentageValues
            ? qp->positionDivergencePercentageMin * value / 1e+2
            : qp->positionDivergenceMin;
        if (mPDivMode::Manual == qp->positionDivergenceMode) positionDivergence  = pDiv;
        else if (mPDivMode::Linear == qp->positionDivergenceMode) positionDivergence  = pDivMin + (divCenter * (pDiv - pDivMin));
        else if (mPDivMode::Sine == qp->positionDivergenceMode) positionDivergence  = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
        else if (mPDivMode::SQRT == qp->positionDivergenceMode) positionDivergence  = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
        else if (mPDivMode::Switch == qp->positionDivergenceMode) positionDivergence  = divCenter < 1e-1 ? pDivMin : pDiv;
      }
  };
}

#endif
