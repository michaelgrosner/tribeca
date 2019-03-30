#ifndef K_UNITS_H_
#define K_UNITS_H_

namespace ₿ {
  SCENARIO("general") {
    GIVEN("Connectivity") {
      Connectivity on, off;
      WHEN("assigned") {
        REQUIRE_NOTHROW(on  = Connectivity::Connected);
        REQUIRE_NOTHROW(off = Connectivity::Disconnected);
        THEN("values") {
          REQUIRE_FALSE(!(bool)on);
          REQUIRE(!(bool)off);
          THEN("combined") {
            REQUIRE(      ((bool)on  and (bool)on));
            REQUIRE_FALSE(((bool)on  and (bool)off));
            REQUIRE_FALSE(((bool)off and (bool)on));
            REQUIRE_FALSE(((bool)off and (bool)off));
          }
        }
      }
    }
  }

  SCENARIO("BTC/EUR") {
    Print::display = nullptr;
    GIVEN("mLevel") {
      mLevel level;
      WHEN("defaults") {
        REQUIRE_NOTHROW(level = {});
        THEN("empty") {
          REQUIRE((!level.size or !level.price));
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(level = {
          1234.56,
          0.12345678
        });
        THEN("values") {
          REQUIRE(level.price == 1234.56);
          REQUIRE(level.size == 0.12345678);
        }
        THEN("not empty") {
          REQUIRE_FALSE((!level.size or !level.price));
        }
        THEN("to json") {
          REQUIRE(((json)level).dump() == "{"
            "\"price\":1234.56,"
            "\"size\":0.12345678"
          "}");
        }
        WHEN("clear") {
          REQUIRE_NOTHROW(level.price = level.size = 0);
          THEN("values") {
            REQUIRE_FALSE(level.price);
            REQUIRE_FALSE(level.size);
          }
          THEN("empty") {
            REQUIRE((!level.size or !level.price));
          }
          THEN("to json") {
            REQUIRE(((json)level).dump() == "{\"price\":0.0}");
          }
        }
      }
    }
    GIVEN("mLevels") {
      mLevels levels;
      WHEN("defaults") {
        REQUIRE_NOTHROW(levels = mLevels());
        THEN("empty") {
          REQUIRE((levels.bids.empty() or levels.asks.empty()));
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(levels = {
          { {1234.56, 0.12345678} },
          { {1234.57, 0.12345679} }
        });
        THEN("values") {
          REQUIRE(levels.asks.cbegin()->price - levels.bids.cbegin()->price == Approx(0.01));
        }
        THEN("not empty") {
          REQUIRE_FALSE((levels.bids.empty() or levels.asks.empty()));
        }
        THEN("to json") {
          REQUIRE(((json)levels).dump() == "{"
            "\"asks\":[{\"price\":1234.57,\"size\":0.12345679}],"
            "\"bids\":[{\"price\":1234.56,\"size\":0.12345678}]"
          "}");
        }
        WHEN("clear") {
          REQUIRE_NOTHROW(levels.bids.clear());
          REQUIRE_NOTHROW(levels.asks.clear());
          THEN("empty") {
            REQUIRE((levels.bids.empty() or levels.asks.empty()));
          }
          THEN("to json") {
            REQUIRE(((json)levels).dump() == "{"
              "\"asks\":[],"
              "\"bids\":[]"
            "}");
          }
        }
      }
    }
    GIVEN("mMarketLevels") {
      KryptoNinja K;
      K.gateway = Gw::new_Gw("NULL");
      K.gateway->minTick = 0.01;
      K.gateway->minSize = 0.001;
      mOrders orders(K);
      mQuotingParams qp(K);
      mMarketLevels levels(K, qp, orders);
      WHEN("defaults") {
        THEN("fair value") {
          REQUIRE_FALSE(levels.fairValue);
          REQUIRE_NOTHROW(levels.stats.fairPrice.read = [&]() {
            REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":0.0}");
          });
          REQUIRE_FALSE(levels.ready());
          REQUIRE_FALSE(levels.fairValue);
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(levels.diff.read = [&]() {
          FAIL("diff.broadcast() before diff.hello()");
        });
        REQUIRE_NOTHROW(levels.stats.fairPrice.read = [&]() {
          REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":1234.55}");
        });
        REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::BBO);
        vector<RandId> randIds;
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.52, 0.34567890, Tstamp, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.52, 0.23456789, Tstamp, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.55, 0.01234567, Tstamp, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Ask, 1234.69, 0.01234568, Tstamp, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(levels.read_from_gw({
          { {1234.50, 0.12345678}, {1234.55, 0.01234567} },
          { {1234.60, 1.23456789}, {1234.69, 0.11234569} }
        }));
        THEN("filters") {
          REQUIRE(levels.bids.size() == 1);
          REQUIRE(levels.bids[0].price == 1234.50);
          REQUIRE(levels.bids[0].size  == 0.12345678);
          REQUIRE(levels.asks.size() == 2);
          REQUIRE(levels.asks[0].price == 1234.60);
          REQUIRE(levels.asks[0].size  == 1.23456789);
          REQUIRE(levels.asks[1].price == 1234.69);
          REQUIRE(levels.asks[1].size  == 0.10000001);
          REQUIRE(levels.unfiltered.bids.size() == 2);
          REQUIRE(levels.unfiltered.bids[0].price == 1234.50);
          REQUIRE(levels.unfiltered.bids[0].size  == 0.12345678);
          REQUIRE(levels.unfiltered.bids[1].price == 1234.55);
          REQUIRE(levels.unfiltered.bids[1].size  == 0.01234567);
          REQUIRE(levels.unfiltered.asks.size() == 2);
          REQUIRE(levels.unfiltered.asks[0].price == 1234.60);
          REQUIRE(levels.unfiltered.asks[0].size  == 1.23456789);
          REQUIRE(levels.unfiltered.asks[1].price == 1234.69);
          REQUIRE(levels.unfiltered.asks[1].size  == 0.11234569);
        }
        THEN("fair value") {
          REQUIRE_NOTHROW(levels.stats.fairPrice.read = []() {
            FAIL("broadcast() while filtering");
          });
          REQUIRE(levels.ready());
          REQUIRE(levels.fairValue == 1234.55);
        }
        THEN("fair value weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::wBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.read = [&]() {
            FAIL("broadcast() while filtering");
          });
          REQUIRE(levels.ready());
          REQUIRE(levels.fairValue == 1234.59);
        }
        THEN("fair value reversed weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::rwBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.read = [&]() {
            FAIL("broadcast() while filtering");
          });
          REQUIRE(levels.ready());
          REQUIRE(levels.fairValue == 1234.51);
        }
        WHEN("diff") {
          REQUIRE(levels.diff.empty());
          REQUIRE(levels.diff.hello().dump() == "[{"
            "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234569}],"
            "\"bids\":[{\"price\":1234.5,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
          "}]");
          REQUIRE_FALSE(levels.diff.empty());
          THEN("broadcast") {
            REQUIRE_NOTHROW(qp.delayUI = 0);
            this_thread::sleep_for(chrono::milliseconds(370));
            REQUIRE_NOTHROW(levels.diff.read = [&]() {
              REQUIRE(levels.diff.blob().dump() == "{"
                "\"asks\":[{\"price\":1234.69,\"size\":0.11234566}],"
                "\"bids\":[{\"price\":1234.5},{\"price\":1234.4,\"size\":0.12345678}],"
                "\"diff\":true"
              "}");
            });
            REQUIRE_NOTHROW(levels.stats.fairPrice.read = [&]() {
              REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":1234.5}");
            });
            REQUIRE_NOTHROW(levels.read_from_gw({
              { {1234.40, 0.12345678}, {1234.55, 0.01234567} },
              { {1234.60, 1.23456789}, {1234.69, 0.11234566} }
              }));
            REQUIRE(levels.diff.hello().dump() == "[{"
              "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234566}],"
              "\"bids\":[{\"price\":1234.4,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
            "}]");
          }
        }
      }
    }

    GIVEN("mRecentTrades") {
      KryptoNinja K;
      K.gateway = Gw::new_Gw("NULL");
      K.gateway->minTick = 0.01;
      K.gateway->minSize = 0.001;
      mQuotingParams qp(K);
      mRecentTrades recentTrades(qp);
      WHEN("defaults") {
        THEN("empty") {
          REQUIRE_FALSE(recentTrades.lastBuyPrice);
          REQUIRE_FALSE(recentTrades.lastSellPrice);
          REQUIRE_FALSE(recentTrades.buys.size());
          REQUIRE_FALSE(recentTrades.sells.size());
          REQUIRE_FALSE(recentTrades.sumBuys);
          REQUIRE_FALSE(recentTrades.sumSells);
        }
      }
      WHEN("assigned") {
        mLastOrder order = {};
        REQUIRE_NOTHROW(order.price = 1234.57);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.01234566);
        REQUIRE_NOTHROW(order.side = Side::Ask);
        REQUIRE_NOTHROW(recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.58);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.01234567);
        REQUIRE_NOTHROW(recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.56);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345678);
        REQUIRE_NOTHROW(order.side = Side::Bid);
        REQUIRE_NOTHROW(recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.50);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345679);
        REQUIRE_NOTHROW(recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.60);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345678);
        REQUIRE_NOTHROW(order.side = Side::Ask);
        REQUIRE_NOTHROW(recentTrades.insert(order));
        THEN("values") {
          REQUIRE(recentTrades.lastBuyPrice == 1234.50);
          REQUIRE(recentTrades.lastSellPrice == 1234.60);
          REQUIRE(recentTrades.buys.size() == 2);
          REQUIRE(recentTrades.sells.size() == 3);
          REQUIRE_FALSE(recentTrades.sumBuys);
          REQUIRE_FALSE(recentTrades.sumSells);
          WHEN("reset") {
            THEN("skip") {
              REQUIRE_NOTHROW(recentTrades.expire());
              REQUIRE(recentTrades.lastBuyPrice == 1234.50);
              REQUIRE(recentTrades.lastSellPrice == 1234.60);
              REQUIRE(recentTrades.buys.size() == 1);
              REQUIRE_FALSE(recentTrades.sells.size());
              REQUIRE(recentTrades.sumBuys == 0.09876546);
              REQUIRE_FALSE(recentTrades.sumSells);
            }
            THEN("expired") {
              this_thread::sleep_for(chrono::milliseconds(1001));
              REQUIRE_NOTHROW(qp.tradeRateSeconds = 1);
              REQUIRE_NOTHROW(recentTrades.expire());
              REQUIRE(recentTrades.lastBuyPrice == 1234.50);
              REQUIRE(recentTrades.lastSellPrice == 1234.60);
              REQUIRE_FALSE(recentTrades.buys.size());
              REQUIRE_FALSE(recentTrades.sells.size());
              REQUIRE_FALSE(recentTrades.sumBuys);
              REQUIRE_FALSE(recentTrades.sumSells);
            }
          }
        }
      }
    }

    GIVEN("mEwma") {
      KryptoNinja K;
      K.gateway = Gw::new_Gw("NULL");
      K.gateway->minTick = 0.01;
      K.gateway->minSize = 0.001;
      mQuotingParams qp(K);
      Price fairValue = 0;
      mEwma ewma(K, fairValue, qp);
      WHEN("defaults") {
        REQUIRE_FALSE(ewma.mgEwmaM);
      }
      WHEN("assigned") {
        vector<Price> fairHistory = { 268.05, 258.73, 239.82, 250.21, 224.49, 242.53, 248.25, 270.58, 252.77, 273.55,
                                      255.90, 226.10, 225.00, 263.12, 218.36, 254.73, 218.65, 252.40, 296.10, 222.20 };
        REQUIRE_NOTHROW(ewma.fairValue96h.Backup::push = ewma.Backup::push = [&]() {
          INFO("push()");
        });
        for (const Price &it : fairHistory) {
          REQUIRE_NOTHROW(fairValue = it);
          REQUIRE_NOTHROW(ewma.timer_60s(0));
        };
        REQUIRE_NOTHROW(qp.mediumEwmaPeriods = 20);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 0);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 1);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 2);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 3);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 4);
        REQUIRE_NOTHROW(qp._diffEwma |= true << 5);
        REQUIRE_NOTHROW(K.clicked(&qp));
        THEN("values") {
          REQUIRE(ewma.mgEwmaVL == Approx(266.1426832796));
          REQUIRE(ewma.mgEwmaL == Approx(264.4045182289));
          REQUIRE(ewma.mgEwmaM == Approx(249.885114711));
          REQUIRE(ewma.mgEwmaS == Approx(256.7706209412));
          REQUIRE(ewma.mgEwmaXS == Approx(247.5567169778));
          REQUIRE(ewma.mgEwmaU == Approx(245.5969655991));
          REQUIRE(ewma.lifetime() == 24000000);
          THEN("to json") {
            REQUIRE(((json)ewma).dump() == "{"
              "\"ewmaExtraShort\":247.55671697778087,"
              "\"ewmaLong\":264.40451822891674,"
              "\"ewmaMedium\":249.88511471099878,"
              "\"ewmaQuote\":264.40451822891674,"
              "\"ewmaShort\":256.7706209412057,"
              "\"ewmaTrendDiff\":-0.7916373276580089,"
              "\"ewmaUltraShort\":245.59696559906004,"
              "\"ewmaVeryLong\":266.142683279562,"
              "\"ewmaWidth\":0.0"
            "}");
          }
        }
      }
    }

    GIVEN("mBroker") {
      KryptoNinja K;
      K.gateway = Gw::new_Gw("NULL");
      K.gateway->minTick = 0.01;
      mButtons button(K);
      mOrders orders(K);
      mQuotingParams qp(K);
      mMarketLevels levels(K, qp, orders);
      levels.fairValue = 500;
      mWalletPosition wallet(K, qp, orders, button, levels);
      wallet.base = {"BTC", 1, 0};
      wallet.quote = {"EUR", 1000, 0};
      mBroker broker(K, qp, orders, button, levels, wallet);
      WHEN("assigned") {
        vector<RandId> randIds;
        const Clock time = Tstamp;
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.50, 0.12345678, time-69, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.51, 0.12345679, time-69, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Bid, 1234.52, 0.12345680, time-69, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Ask, 1234.50, 0.12345678, time-69, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Ask, 1234.51, 0.12345679, time-69, false, randIds.back()}));
        REQUIRE_NOTHROW(orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert({Side::Ask, 1234.52, 0.12345680, time, false, randIds.back()}));
        THEN("held amount") {
          REQUIRE_NOTHROW(wallet.profits.Backup::push = [&]() {
            INFO("push()");
          });
          bool askForFees = false;
          mLastOrder order = {};
          REQUIRE_NOTHROW(order.price = 1);
          REQUIRE_NOTHROW(order.side = Side::Ask);
          REQUIRE_NOTHROW(wallet.calcFundsAfterOrder(order, &askForFees));
          REQUIRE_NOTHROW(order.side = Side::Bid);
          REQUIRE_NOTHROW(wallet.calcFundsAfterOrder(order, &askForFees));
          REQUIRE(wallet.base.held == 0.37037037);
          REQUIRE(wallet.quote.held == Approx(457.22592546));
        }
        THEN("to json") {
          REQUIRE(string::npos == orders.blob().dump().find("\"status\":0"));
          REQUIRE(string::npos == orders.blob().dump().find("\"status\":2"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"disablePostOnly\":false,\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[0] + "\",\"price\":1234.5,\"quantity\":0.12345678,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"disablePostOnly\":false,\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[1] + "\",\"price\":1234.51,\"quantity\":0.12345679,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"disablePostOnly\":false,\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[2] + "\",\"price\":1234.52,\"quantity\":0.1234568,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"disablePostOnly\":false,\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[3] + "\",\"price\":1234.5,\"quantity\":0.12345678,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"disablePostOnly\":false,\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[4] + "\",\"price\":1234.51,\"quantity\":0.12345679,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
        }
      }
    }
  }

  SCENARIO("bugs") {
    GIVEN("#909 cumulated cross pongs") {
      KryptoNinja K;
      K.gateway = Gw::new_Gw("NULL");
      K.gateway->minTick = 0.01;
      K.gateway->report({}, true);
      mQuotingParams q(K);
      q.from_json(R"({"aggressivePositionRebalancing":1,"aprMultiplier":3.0,"audio":false,"autoPositionMode":0,"bestWidth":true,"bestWidthSize":0.0,"bullets":2,"buySize":0.02,"buySizeMax":false,"buySizePercentage":1.0,"cancelOrdersAuto":false,"cleanPongsAuto":0.0,"delayUI":3,"ewmaSensiblityPercentage":0.5,"extraShortEwmaPeriods":12,"fvModel":0,"localBalance":true,"longEwmaPeriods":200,"mediumEwmaPeriods":100,"mode":0,"percentageValues":true,"pingAt":0,"pongAt":1,"positionDivergence":0.9,"positionDivergenceMin":0.4,"positionDivergenceMode":0,"positionDivergencePercentage":50.0,"positionDivergencePercentageMin":10.0,"profitHourInterval":72.0,"protectionEwmaPeriods":5,"protectionEwmaQuotePrice":false,"protectionEwmaWidthPing":false,"quotingEwmaTrendProtection":false,"quotingEwmaTrendThreshold":2.0,"quotingStdevBollingerBands":false,"quotingStdevProtection":0,"quotingStdevProtectionFactor":1.0,"quotingStdevProtectionPeriods":1200,"range":0.5,"rangePercentage":5.0,"safety":3,"sellSize":0.01,"sellSizeMax":false,"sellSizePercentage":1.0,"shortEwmaPeriods":50,"sopSizeMultiplier":2.0,"sopTradesMultiplier":2.0,"sopWidthMultiplier":2.0,"superTrades":0,"targetBasePosition":1.0,"targetBasePositionPercentage":50.0,"tradeRateSeconds":3,"tradesPerMinute":0.9,"ultraShortEwmaPeriods":3,"veryLongEwmaPeriods":400,"widthPercentage":false,"widthPing":0.01,"widthPingPercentage":0.25,"widthPong":0.01,"widthPongPercentage":0.25})"_json);
      mButtons b(K);
      mTradesHistory trades(K, q, b);

      auto parseTrade = [](string line)->mLastOrder {
        stringstream ss(line);
        string _, pingpong, side;
        mLastOrder order;
        ss >> _ >> _ >> _ >> _ >> pingpong >> _ >> side >> order.tradeQuantity >> _ >> _ >> _ >> order.price;
        order.side = (side == "BUY" ? Side::Bid : Side::Ask);
        order.isPong = (pingpong == "PONG");
        return order;
      };
      
      std::vector<mLastOrder> orders({
          parseTrade("03/30 07:10:34.800532 GW COINBASE PING TRADE BUY  0.03538069 ETH at price 141.31 USD (value 4.99 USD)."),
          parseTrade("03/30 07:12:10.769009 GW COINBASE PONG TRADE SELL 0.03241380 ETH at price 141.40 USD (value 4.58 USD)."),
          parseTrade("03/30 07:12:10.786990 GW COINBASE PONG TRADE SELL 0.00295204 ETH at price 141.40 USD (value 0.41 USD)."),
          parseTrade("03/30 07:25:49.333540 GW COINBASE PING TRADE BUY  0.03528853 ETH at price 141.60 USD (value 4.99 USD)."),
          parseTrade("03/30 07:25:50.787607 GW COINBASE PONG TRADE SELL 0.03528853 ETH at price 141.66 USD (value 4.99 USD)."),
          parseTrade("03/30 07:38:07.369008 GW COINBASE PING TRADE BUY  0.03528867 ETH at price 141.66 USD (value 4.99 USD)."),
          parseTrade("03/30 07:38:34.268582 GW COINBASE PONG TRADE SELL 0.03529119 ETH at price 141.68 USD (value 5.00 USD)."),
          parseTrade("03/30 07:43:02.229028 GW COINBASE PING TRADE BUY  0.03528870 ETH at price 141.63 USD (value 4.99 USD)."),
          parseTrade("03/30 07:45:22.735730 GW COINBASE PING TRADE SELL 0.03530102 ETH at price 141.55 USD (value 4.99 USD)."),
          parseTrade("03/30 08:14:52.978466 GW COINBASE PING TRADE BUY  0.03512242 ETH at price 142.28 USD (value 4.99 USD)."),
          parseTrade("03/30 08:15:13.002363 GW COINBASE PING TRADE BUY  0.03515685 ETH at price 142.22 USD (value 5.00 USD).")
      });

      Amount expectedBaseDelta = 0;
      Amount expectedQuoteDelta = 0;
      Amount baseSign;
      Clock lastTime = Tstamp;
      for (mLastOrder const & order : orders) {
        baseSign = (order.side == Side::Bid) ? 1 : -1;
        expectedBaseDelta += baseSign * order.tradeQuantity;
        expectedQuoteDelta -= baseSign * order.tradeQuantity * order.price;
        Clock time;
        while ((time = Tstamp) == lastTime);
        trades.insert(order);
        lastTime = time;

        Amount actualBaseDelta = 0;
        Amount actualQuoteDelta = 0;
        Amount expectedDiff = 0;
        Amount actualDiff = 0;

        for (mOrderFilled const & trade : trades) {
          baseSign = (trade.side == Side::Bid) ? 1 : -1;
          actualBaseDelta += baseSign * (trade.quantity - trade.Kqty);
          Amount diff = baseSign * (trade.Kvalue - trade.value);
          actualQuoteDelta += diff;
          if (trade.Kdiff) {
            actualDiff += diff;
            expectedDiff += trade.Kdiff;
          }
        }

        REQUIRE(abs(actualBaseDelta - expectedBaseDelta) < 0.000000000001);
        REQUIRE(abs(actualQuoteDelta - expectedQuoteDelta) < 0.000000000001);
        REQUIRE(abs(actualDiff - expectedDiff) < 0.000000000001);
      }
    }
  }
}

#endif
