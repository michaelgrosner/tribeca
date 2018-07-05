#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass,
            public Wallet { public: PG() { wallet = this; };
    protected:
      void load() {
        sqlite->backup(&position.target);
        sqlite->backup(&position.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          position.balance.reset(rawdata);
          position.send_ratelimit(market->levels);
        });
      };
      void waitSysAdmin() {
        screen->printme(&position.target);
      };
      void waitWebAdmin() {
        client->welcome(position.target);
        client->welcome(position.safety);
        client->welcome(position);
      };
    public:
      void timer_1s() {
        calcSafety();
      };
      void calcSafety() {
        if (position.empty() or market->levels.empty()) return;
        position.calcSafetySizes();
        position.safety.send_ratelimit(nextSafety(market->levels.fairValue));
      };
    private:
      mSafety nextSafety(const mPrice &fv) {
        map<mPrice, mTrade> tradesBuy;
        map<mPrice, mTrade> tradesSell;
        for (mTrade &it: broker->orders.tradesHistory)
          (it.side == mSide::Bid ? tradesBuy : tradesSell)[it.price] = it;
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * fv / 100
          : qp.widthPong;
        mPrice buyPing = 0,
               sellPing = 0;
        mAmount buyQty = 0,
                sellQty = 0;
        if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
          matchBestPing(fv, &tradesBuy, &buyPing, &buyQty, position.safety.sellSize, widthPong, true);
          matchBestPing(fv, &tradesSell, &sellPing, &sellQty, position.safety.buySize, widthPong);
          if (!buyQty) matchFirstPing(fv, &tradesBuy, &buyPing, &buyQty, position.safety.sellSize, widthPong*-1, true);
          if (!sellQty) matchFirstPing(fv, &tradesSell, &sellPing, &sellQty, position.safety.buySize, widthPong*-1);
        } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
          matchLastPing(fv, &tradesBuy, &buyPing, &buyQty, position.safety.sellSize, widthPong);
          matchLastPing(fv, &tradesSell, &sellPing, &sellQty, position.safety.buySize, widthPong, true);
        } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
          matchAllPing(fv, &tradesBuy, &buyPing, &buyQty, position.safety.sellSize, widthPong);
          matchAllPing(fv, &tradesSell, &sellPing, &sellQty, position.safety.buySize, widthPong);
        }
        if (buyQty) buyPing /= buyQty;
        if (sellQty) sellPing /= sellQty;
        position.safety.recentTrades.reset();
        return mSafety(
          position.safety.recentTrades.sumBuys / position.safety.buySize,
          position.safety.recentTrades.sumSells / position.safety.sellSize,
          (position.safety.recentTrades.sumBuys + position.safety.recentTrades.sumSells) / (position.safety.buySize + position.safety.sellSize),
          buyPing,
          sellPing
        );
      };
      void matchFirstPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, true, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, false, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(false, true, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width) {
        matchPing(false, false, fv, trades, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<mPrice, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fv, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<mPrice, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fv, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
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
