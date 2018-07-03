#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass,
            public Wallet { public: PG() { wallet = this; };
    protected:
      void load() {
        sqlite->backup(&target);
        sqlite->backup(&position.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          position.balance.reset(rawdata);
          calcWallet();
        });
      };
      void waitSysAdmin() {
        screen->printme(&position);
      };
      void waitWebAdmin() {
        client->welcome(position);
        client->welcome(safety);
        client->welcome(target);
      };
    public:
      void timer_1s() {
        calcSafety();
      };
      void timer_60s() {
        calcTargetBasePos();
      };
      void calcSafety() {
        if (position.empty() or market->levels.empty()) return;
        safety.send_ratelimit(nextSafety());
      };
      void calcWallet() {
        if (position.balance.empty() or market->levels.empty()) return;
        if (args.maxWallet) position.balance.calcMaxWallet(market->levels.fairValue);
        position.send_ratelimit(market->levels.fairValue, mPair(gw->base, gw->quote));
        calcTargetBasePos();
      };
    private:
      void calcTargetBasePos() {                                    PRETTY_DEBUG
        if (position.warn_empty()) return;
        mAmount baseValue = position.baseValue,
                prev = target.targetBasePosition,
                next = qp.autoPositionMode == mAutoPositionMode::Manual
                         ? (qp.percentageValues
                           ? qp.targetBasePositionPercentage * baseValue / 1e+2
                           : qp.targetBasePosition)
                         : market->levels.stats.ewma.targetPositionAutoPercentage * baseValue / 1e+2;
        if (prev and abs(prev - next) < 1e-4 and target.sideAPRDiff == target.sideAPR) return;
        target.targetBasePosition = next;
        target.sideAPRDiff = target.sideAPR;
        target.calcPDiv(baseValue);
        target.send_push();
        if (!args.debugWallet) return;
        screen->log("PG", "TBP: "
          + to_string((int)(target.targetBasePosition / baseValue * 1e+2)) + "% = " + FN::str8(target.targetBasePosition)
          + " " + gw->base + ", pDiv: "
          + to_string((int)(target.positionDivergence  / baseValue * 1e+2)) + "% = " + FN::str8(target.positionDivergence)
          + " " + gw->base);
      };
      mSafety nextSafety() {
        mAmount buySize = qp.percentageValues
          ? qp.buySizePercentage * position.baseValue / 100
          : qp.buySize;
        mAmount sellSize = qp.percentageValues
          ? qp.sellSizePercentage * position.baseValue / 100
          : qp.sellSize;
        map<mPrice, mTrade> tradesBuy;
        map<mPrice, mTrade> tradesSell;
        for (mTrade &it: broker->tradesHistory) {
          (it.side == mSide::Bid ? tradesBuy : tradesSell)[it.price] = it;
          if (qp.safety == mQuotingSafety::PingPong)
            (it.side == mSide::Ask ? buySize : sellSize) = it.quantity;
        }
        mAmount totalBasePosition = position.baseAmount + position.baseHeldAmount;
        if (qp.aggressivePositionRebalancing != mAPR::Off) {
          if (qp.buySizeMax) buySize = fmax(buySize, target.targetBasePosition - totalBasePosition);
          if (qp.sellSizeMax) sellSize = fmax(sellSize, totalBasePosition - target.targetBasePosition);
        }
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * market->levels.fairValue / 100
          : qp.widthPong;
        mPrice buyPing = 0,
               sellPing = 0;
        mAmount buyQty = 0,
                sellQty = 0;
        if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
          matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
          matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
          if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
        } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
          matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
        } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
          matchAllPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
          matchAllPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPing /= sellQty;
        safety.recentTrades.reset();
        return mSafety(
          safety.recentTrades.sumBuys / buySize,
          safety.recentTrades.sumSells / sellSize,
          (safety.recentTrades.sumBuys + safety.recentTrades.sumSells) / (buySize + sellSize),
          buyPing,
          sellPing,
          buySize,
          sellSize
        );
      };
      void matchFirstPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width) {
        matchPing(false, false, trades, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<mPrice, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * market->levels.fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<mPrice, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * market->levels.fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      bool matchPing(bool _near, bool _far, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, mPrice fv, mPrice price, mAmount qtyTrade, mPrice priceTrade, mAmount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and (!qp._matchPings or KqtyTrade < qtyTrade)
        ) {
          mAmount qty_ = qtyTrade;
          if (_near or _far)
            qty_ = fmin(qtyMax - *qty, qty_);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax and (_near or _far);
      };
  };
}

#endif
