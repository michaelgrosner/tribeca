#ifndef K_UNITS_H_
#define K_UNITS_H_
//! \file
//! \brief Collection of test units.
//! \note  Test units about benchmarks are removed after a while,
//!        but if you need some examples, see https://github.com/catchorg/Catch2/blob/master/projects/SelfTest/UsageTests/Benchmark.tests.cpp

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
  }

  class BTCEUR {
    protected:
      static     KryptoNinja K;
      static  mQuotingParams qp;
      static         mOrders orders;
      static        mButtons button;
      static   mMarketLevels levels;
      static mWalletPosition wallet;
      static         mBroker broker;
      static         mMemory memory;
    public:
      BTCEUR()
      {
        if (K.gateway) return;
        silent();
        K.gateway = Gw::new_Gw("NULL");
        K.gateway->exchange = "NULL";
        K.gateway->base     = "BTC";
        K.gateway->quote    = "EUR";
        K.gateway->minTick  = 0.01;
        K.gateway->minSize  = 0.001;
        K.gateway->report({}, false);
        K.gateway->write_Connectivity = [&](const Connectivity &rawdata) {
          broker.semaphore.read_from_gw(rawdata);
        };
      };
    protected:
      void verbose() {
        K.display = nullptr;
      };
      void silent() {
        K.display = []() {};
      };
  };

      KryptoNinja BTCEUR::K;
   mQuotingParams BTCEUR::qp(BTCEUR::K);
          mOrders BTCEUR::orders(BTCEUR::K);
         mButtons BTCEUR::button(BTCEUR::K);
    mMarketLevels BTCEUR::levels(BTCEUR::K, BTCEUR::qp, BTCEUR::orders);
  mWalletPosition BTCEUR::wallet(BTCEUR::K, BTCEUR::qp, BTCEUR::orders, BTCEUR::button, BTCEUR::levels);
          mBroker BTCEUR::broker(BTCEUR::K, BTCEUR::qp, BTCEUR::orders, BTCEUR::button, BTCEUR::levels, BTCEUR::wallet);
          mMemory BTCEUR::memory(BTCEUR::K);

  SCENARIO_METHOD(BTCEUR, "expected") {
    GIVEN("mMarketLevels") {
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
        for (mOrder *const it : orders.working())
          orders.purge(it);
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
      REQUIRE_NOTHROW(wallet.safety.recentTrades.lastBuyPrice = 0);
      REQUIRE_NOTHROW(wallet.safety.recentTrades.lastSellPrice = 0);
      REQUIRE_NOTHROW(wallet.safety.recentTrades.buys = {});
      REQUIRE_NOTHROW(wallet.safety.recentTrades.sells = {});
      REQUIRE_NOTHROW(wallet.safety.recentTrades.sumBuys = 0);
      REQUIRE_NOTHROW(wallet.safety.recentTrades.sumSells = 0);
      WHEN("defaults") {
        THEN("empty") {
          REQUIRE_FALSE(wallet.safety.recentTrades.lastBuyPrice);
          REQUIRE_FALSE(wallet.safety.recentTrades.lastSellPrice);
          REQUIRE_FALSE(wallet.safety.recentTrades.buys.size());
          REQUIRE_FALSE(wallet.safety.recentTrades.sells.size());
          REQUIRE_FALSE(wallet.safety.recentTrades.sumBuys);
          REQUIRE_FALSE(wallet.safety.recentTrades.sumSells);
        }
      }
      WHEN("assigned") {
        mLastOrder order = {};
        REQUIRE_NOTHROW(order.price = 1234.57);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.01234566);
        REQUIRE_NOTHROW(order.side = Side::Ask);
        REQUIRE_NOTHROW(wallet.safety.recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.58);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.01234567);
        REQUIRE_NOTHROW(wallet.safety.recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.56);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345678);
        REQUIRE_NOTHROW(order.side = Side::Bid);
        REQUIRE_NOTHROW(wallet.safety.recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.50);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345679);
        REQUIRE_NOTHROW(wallet.safety.recentTrades.insert(order));
        REQUIRE_NOTHROW(order.price = 1234.60);
        REQUIRE_NOTHROW(order.tradeQuantity = 0.12345678);
        REQUIRE_NOTHROW(order.side = Side::Ask);
        REQUIRE_NOTHROW(wallet.safety.recentTrades.insert(order));
        THEN("values") {
          REQUIRE(wallet.safety.recentTrades.lastBuyPrice == 1234.50);
          REQUIRE(wallet.safety.recentTrades.lastSellPrice == 1234.60);
          REQUIRE(wallet.safety.recentTrades.buys.size() == 2);
          REQUIRE(wallet.safety.recentTrades.sells.size() == 3);
          REQUIRE_FALSE(wallet.safety.recentTrades.sumBuys);
          REQUIRE_FALSE(wallet.safety.recentTrades.sumSells);
          WHEN("reset") {
            THEN("skip") {
              REQUIRE_NOTHROW(wallet.safety.recentTrades.expire());
              REQUIRE(wallet.safety.recentTrades.lastBuyPrice == 1234.50);
              REQUIRE(wallet.safety.recentTrades.lastSellPrice == 1234.60);
              REQUIRE(wallet.safety.recentTrades.buys.size() == 1);
              REQUIRE_FALSE(wallet.safety.recentTrades.sells.size());
              REQUIRE(wallet.safety.recentTrades.sumBuys == 0.09876546);
              REQUIRE_FALSE(wallet.safety.recentTrades.sumSells);
            }
            THEN("expired") {
              this_thread::sleep_for(chrono::milliseconds(1001));
              REQUIRE_NOTHROW(qp.tradeRateSeconds = 1);
              REQUIRE_NOTHROW(wallet.safety.recentTrades.expire());
              REQUIRE(wallet.safety.recentTrades.lastBuyPrice == 1234.50);
              REQUIRE(wallet.safety.recentTrades.lastSellPrice == 1234.60);
              REQUIRE_FALSE(wallet.safety.recentTrades.buys.size());
              REQUIRE_FALSE(wallet.safety.recentTrades.sells.size());
              REQUIRE_FALSE(wallet.safety.recentTrades.sumBuys);
              REQUIRE_FALSE(wallet.safety.recentTrades.sumSells);
            }
          }
        }
      }
    }

    GIVEN("mEwma") {
      levels.fairValue = 0;
      WHEN("defaults") {
        REQUIRE_FALSE(levels.stats.ewma.mgEwmaM);
      }
      WHEN("assigned") {
        vector<Price> fairHistory = { 268.05, 258.73, 239.82, 250.21, 224.49, 242.53, 248.25, 270.58, 252.77, 273.55,
                                      255.90, 226.10, 225.00, 263.12, 218.36, 254.73, 218.65, 252.40, 296.10, 222.20 };
        REQUIRE_NOTHROW(levels.stats.ewma.fairValue96h.Backup::push = levels.stats.ewma.Backup::push = [&]() {
          INFO("push()");
        });
        for (const Price &it : fairHistory) {
          REQUIRE_NOTHROW(levels.fairValue = it);
          REQUIRE_NOTHROW(levels.stats.ewma.timer_60s(0));
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
          REQUIRE(levels.stats.ewma.mgEwmaVL == Approx(266.1426832796));
          REQUIRE(levels.stats.ewma.mgEwmaL == Approx(264.4045182289));
          REQUIRE(levels.stats.ewma.mgEwmaM == Approx(249.885114711));
          REQUIRE(levels.stats.ewma.mgEwmaS == Approx(256.7706209412));
          REQUIRE(levels.stats.ewma.mgEwmaXS == Approx(247.5567169778));
          REQUIRE(levels.stats.ewma.mgEwmaU == Approx(245.5969655991));
          REQUIRE(levels.stats.ewma.lifetime() == 24000000);
          THEN("to json") {
            REQUIRE(((json)levels.stats.ewma).dump() == "{"
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
      REQUIRE_NOTHROW(qp.mode = mQuotingMode::Top);
      REQUIRE_NOTHROW(qp.autoPositionMode = mAutoPositionMode::Manual);
      REQUIRE_NOTHROW(qp.aggressivePositionRebalancing = mAPR::Off);
      REQUIRE_NOTHROW(qp.safety = mQuotingSafety::Off);
      REQUIRE_NOTHROW(qp.protectionEwmaQuotePrice = false);
      REQUIRE_NOTHROW(qp.widthPercentage = false);
      REQUIRE_NOTHROW(qp.percentageValues = false);
      REQUIRE_NOTHROW(qp.widthPing = 1);
      REQUIRE_NOTHROW(qp.bestWidth = true);
      REQUIRE_NOTHROW(qp.protectionEwmaWidthPing = false);
      REQUIRE_NOTHROW(qp.targetBasePosition = 1);
      REQUIRE_NOTHROW(qp.positionDivergence = 1);
      REQUIRE_NOTHROW(qp.read = levels.diff.read = levels.stats.fairPrice.read = wallet.read = wallet.safety.read = wallet.target.read = broker.calculon.read = broker.semaphore.read = [&]() {
        INFO("read()");
      });
      REQUIRE_NOTHROW(qp.Backup::push = wallet.target.Backup::push = wallet.profits.Backup::push = [&]() {
        INFO("push()");
      });
      REQUIRE_NOTHROW(broker.semaphore.read_from_gw(
        Connectivity::Disconnected
      ));
      REQUIRE_NOTHROW(wallet.read_from_gw({
        {"BTC", 1,    0},
        {"EUR", 1000, 0}
      }));
      WHEN("assigned") {
        for (mOrder *const it : orders.working())
          orders.purge(it);
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
          levels.fairValue = 500;
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
      WHEN("ready") {
        REQUIRE_NOTHROW(levels.read_from_gw({
          { },
          { }
        }));
        REQUIRE_NOTHROW(broker.semaphore.click({
          {"agree", 0}
        }));
        REQUIRE_NOTHROW(broker.calculon.quotes.bid.clear(mQuoteState::MissingData));
        REQUIRE_NOTHROW(broker.calculon.quotes.ask.clear(mQuoteState::MissingData));
        REQUIRE(broker.calculon.quotes.bid.empty());
        REQUIRE(broker.calculon.quotes.ask.empty());
        REQUIRE_FALSE(broker.ready());
        REQUIRE(broker.calculon.quotes.bid.state == mQuoteState::Disconnected);
        REQUIRE(broker.calculon.quotes.ask.state == mQuoteState::Disconnected);
        REQUIRE_NOTHROW(broker.clear());
        REQUIRE(broker.calculon.quotes.bid.state == mQuoteState::MissingData);
        REQUIRE(broker.calculon.quotes.ask.state == mQuoteState::MissingData);
        REQUIRE_NOTHROW(K.gateway->ready(nullptr));
        REQUIRE(broker.ready());
        REQUIRE_FALSE(levels.ready());
        REQUIRE_NOTHROW(levels.read_from_gw({ {
          {699, 0.12345678},
          {698, 0.12345678},
          {696, 0.12345678}
        }, {
          {701, 0.12345678},
          {702, 0.12345678},
          {704, 0.12345678}
        } }));
        REQUIRE(levels.fairValue == 700);
        REQUIRE(levels.ready());
        REQUIRE_NOTHROW(wallet.read_from_gw({
          {"BTC", 0, 0},
          {"EUR", 0, 0}
        }));
        REQUIRE_FALSE(wallet.ready());
        REQUIRE_NOTHROW(wallet.read_from_gw({
          {"BTC", 1,    0},
          {"EUR", 1000, 0}
        }));
        REQUIRE_NOTHROW(wallet.safety.timer_1s());
        REQUIRE(wallet.ready());
        REQUIRE_FALSE(broker.calcQuotes());
        THEN("agree") {
          REQUIRE_NOTHROW(qp.click(qp));
          REQUIRE_NOTHROW(broker.semaphore.click({
            {"agree", 1}
          }));
          WHEN("quoting") {
            REQUIRE(broker.calcQuotes());
            REQUIRE_FALSE(broker.calculon.quotes.bid.empty());
            REQUIRE_FALSE(broker.calculon.quotes.ask.empty());
            THEN("to json") {
              REQUIRE(((json)broker.calculon.quotes.bid).dump() == "{"
                "\"price\":699.01,"
                "\"size\":0.02"
              "}");
              REQUIRE(((json)broker.calculon.quotes.ask).dump() == "{"
                "\"price\":700.99,"
                "\"size\":0.01"
              "}");
            }
            WHEN("widthPing=2") {
              REQUIRE_NOTHROW(qp.widthPing = 2);
              REQUIRE(broker.calcQuotes());
              REQUIRE_FALSE(broker.calculon.quotes.bid.empty());
              REQUIRE_FALSE(broker.calculon.quotes.ask.empty());
              THEN("to json") {
                REQUIRE(((json)broker.calculon.quotes.bid).dump() == "{"
                  "\"price\":698.01,"
                  "\"size\":0.02"
                "}");
                REQUIRE(((json)broker.calculon.quotes.ask).dump() == "{"
                  "\"price\":701.99,"
                  "\"size\":0.01"
                "}");
              }
            }
            WHEN("widthPing=3,bestWidth=false") {
              REQUIRE_NOTHROW(qp.bestWidth = false);
              REQUIRE_NOTHROW(qp.widthPing = 3);
              REQUIRE(broker.calcQuotes());
              REQUIRE_FALSE(broker.calculon.quotes.bid.empty());
              REQUIRE_FALSE(broker.calculon.quotes.ask.empty());
              THEN("to json") {
                REQUIRE(((json)broker.calculon.quotes.bid).dump() == "{"
                  "\"price\":698.5,"
                  "\"size\":0.02"
                "}");
                REQUIRE(((json)broker.calculon.quotes.ask).dump() == "{"
                  "\"price\":701.5,"
                  "\"size\":0.01"
                "}");
              }
            }
            WHEN("widthPing=3") {
              REQUIRE_NOTHROW(qp.widthPing = 3);
              REQUIRE(broker.calcQuotes());
              REQUIRE_FALSE(broker.calculon.quotes.bid.empty());
              REQUIRE_FALSE(broker.calculon.quotes.ask.empty());
              THEN("to json") {
                REQUIRE(((json)broker.calculon.quotes.bid).dump() == "{"
                  "\"price\":698.01,"
                  "\"size\":0.02"
                "}");
                REQUIRE(((json)broker.calculon.quotes.ask).dump() == "{"
                  "\"price\":701.99,"
                  "\"size\":0.01"
                "}");
              }
            }
            WHEN("widthPing=4") {
              REQUIRE_NOTHROW(qp.widthPing = 4);
              REQUIRE(broker.calcQuotes());
              REQUIRE_FALSE(broker.calculon.quotes.bid.empty());
              REQUIRE_FALSE(broker.calculon.quotes.ask.empty());
              THEN("to json") {
                REQUIRE(((json)broker.calculon.quotes.bid).dump() == "{"
                  "\"price\":696.01,"
                  "\"size\":0.02"
                "}");
                REQUIRE(((json)broker.calculon.quotes.ask).dump() == "{"
                  "\"price\":703.99,"
                  "\"size\":0.01"
                "}");
              }
            }
          }
        }
      }
    }
  }

  SCENARIO_METHOD(BTCEUR, "bugs") {
    GIVEN("#909 Corrupt Tradehistory Found") {
      qp.click(R"({"aggressivePositionRebalancing":1,"aprMultiplier":3.0,"audio":false,"autoPositionMode":0,"bestWidth":true,"bestWidthSize":0.0,"bullets":2,"buySize":0.02,"buySizeMax":false,"buySizePercentage":1.0,"cancelOrdersAuto":false,"cleanPongsAuto":0.0,"delayUI":3,"ewmaSensiblityPercentage":0.5,"extraShortEwmaPeriods":12,"fvModel":0,"localBalance":true,"longEwmaPeriods":200,"mediumEwmaPeriods":100,"mode":0,"percentageValues":true,"pingAt":0,"pongAt":1,"positionDivergence":0.9,"positionDivergenceMin":0.4,"positionDivergenceMode":0,"positionDivergencePercentage":50.0,"positionDivergencePercentageMin":10.0,"profitHourInterval":72.0,"protectionEwmaPeriods":5,"protectionEwmaQuotePrice":false,"protectionEwmaWidthPing":false,"quotingEwmaTrendProtection":false,"quotingEwmaTrendThreshold":2.0,"quotingStdevBollingerBands":false,"quotingStdevProtection":0,"quotingStdevProtectionFactor":1.0,"quotingStdevProtectionPeriods":1200,"range":0.5,"rangePercentage":5.0,"safety":3,"sellSize":0.01,"sellSizeMax":false,"sellSizePercentage":1.0,"shortEwmaPeriods":50,"sopSizeMultiplier":2.0,"sopTradesMultiplier":2.0,"sopWidthMultiplier":2.0,"superTrades":0,"targetBasePosition":1.0,"targetBasePositionPercentage":50.0,"tradeRateSeconds":3,"tradesPerMinute":0.9,"ultraShortEwmaPeriods":3,"veryLongEwmaPeriods":400,"widthPercentage":false,"widthPing":0.01,"widthPingPercentage":0.25,"widthPong":0.01,"widthPongPercentage":0.25})"_json);
      REQUIRE_NOTHROW(wallet.safety.trades.Backup::push = [&]() {
        INFO("push()");
      });
      REQUIRE_NOTHROW(wallet.safety.trades.read = [&]() {
        INFO("read()");
      });
      auto parseTrade = [](string line)->mLastOrder {
        stringstream ss(line);
        string _, pingpong, side;
        mLastOrder order;
        ss >> _ >> _ >> _ >> _ >> pingpong >> _ >> side >> order.tradeQuantity >> _ >> _ >> _ >> order.price;
        order.side = (side == "BUY" ? Side::Bid : Side::Ask);
        order.isPong = (pingpong == "PONG");
        return order;
      };
      vector<mLastOrder> loglines({
          parseTrade("03/30 07:10:34.800532 GW COINBASE PING TRADE BUY  0.03538069 BTC at price 141.31 EUR (value 4.99 EUR)."),
          parseTrade("03/30 07:12:10.769009 GW COINBASE PONG TRADE SELL 0.03241380 BTC at price 141.40 EUR (value 4.58 EUR)."),
          parseTrade("03/30 07:12:10.786990 GW COINBASE PONG TRADE SELL 0.00295204 BTC at price 141.40 EUR (value 0.41 EUR)."),
          parseTrade("03/30 07:25:49.333540 GW COINBASE PING TRADE BUY  0.03528853 BTC at price 141.60 EUR (value 4.99 EUR)."),
          parseTrade("03/30 07:25:50.787607 GW COINBASE PONG TRADE SELL 0.03528853 BTC at price 141.66 EUR (value 4.99 EUR)."),
          parseTrade("03/30 07:38:07.369008 GW COINBASE PING TRADE BUY  0.03528867 BTC at price 141.66 EUR (value 4.99 EUR)."),
          parseTrade("03/30 07:38:34.268582 GW COINBASE PONG TRADE SELL 0.03529119 BTC at price 141.68 EUR (value 5.00 EUR)."),
          parseTrade("03/30 07:43:02.229028 GW COINBASE PING TRADE BUY  0.03528870 BTC at price 141.63 EUR (value 4.99 EUR)."),
          parseTrade("03/30 07:45:22.735730 GW COINBASE PING TRADE SELL 0.03530102 BTC at price 141.55 EUR (value 4.99 EUR)."),
          parseTrade("03/30 08:14:52.978466 GW COINBASE PING TRADE BUY  0.03512242 BTC at price 142.28 EUR (value 4.99 EUR)."),
          parseTrade("03/30 08:15:13.002363 GW COINBASE PING TRADE BUY  0.03515685 BTC at price 142.22 EUR (value 5.00 EUR).")
      });
      Amount expectedBaseDelta = 0;
      Amount expectedQuoteDelta = 0;
      Amount baseSign;
      WHEN("cumulated cross pongs") {
        for (mLastOrder const &order : loglines) {
          baseSign = (order.side == Side::Bid) ? 1 : -1;
          expectedBaseDelta += baseSign * order.tradeQuantity;
          expectedQuoteDelta -= baseSign * order.tradeQuantity * order.price;
          this_thread::sleep_for(chrono::milliseconds(2));
          wallet.safety.trades.insert(order);
          Amount actualBaseDelta = 0;
          Amount actualQuoteDelta = 0;
          Amount expectedDiff = 0;
          Amount actualDiff = 0;
          for (mOrderFilled const & trade : wallet.safety.trades) {
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
}

#endif
