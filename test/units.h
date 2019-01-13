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
        REQUIRE_NOTHROW(level = mLevel());
        THEN("empty") {
          REQUIRE((!level.size or !level.price));
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(level = mLevel(
          1234.56,
          0.12345678
        ));
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
        REQUIRE_NOTHROW(levels = mLevels(
          { mLevel(1234.56, 0.12345678) },
          { mLevel(1234.57, 0.12345679) }
        ));
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
      Option option;
      mProduct product(option);
      const Price  minTick = 0.01;
      const Amount minSize = 0.001;
      product.minTick = &minTick;
      product.minSize = &minSize;
      mOrders orders(product);
      mMarketLevels levels(product, orders);
      WHEN("defaults") {
        THEN("fair value") {
          REQUIRE_FALSE(levels.fairValue);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":0.0}");
          });
          REQUIRE_FALSE(levels.ready());
          REQUIRE_FALSE(levels.fairValue);
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(levels.diff.mToClient::send = [&]() {
          FAIL("diff.send() before diff.hello()");
        });
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
          REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":1234.55}");
        });
        REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::BBO);
        vector<RandId> randIds;
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.52, 0.34567890, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.52, 0.23456789, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.55, 0.01234567, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Ask, 1234.69, 0.01234568, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(levels.read_from_gw(mLevels(
          { mLevel(1234.50, 0.12345678), mLevel(1234.55, 0.01234567) },
          { mLevel(1234.60, 1.23456789), mLevel(1234.69, 0.11234569) }
        )));
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
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = []() {
            FAIL("send() while filtering");
          });
          REQUIRE(levels.ready());
          REQUIRE(levels.fairValue == 1234.55);
        }
        THEN("fair value weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::wBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            FAIL("send() while filtering");
          });
          REQUIRE(levels.ready());
          REQUIRE(levels.fairValue == 1234.59);
        }
        THEN("fair value reversed weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::rwBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            FAIL("send() while filtering");
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
          THEN("send") {
            REQUIRE_NOTHROW(qp.delayUI = 0);
            this_thread::sleep_for(chrono::milliseconds(370));
            REQUIRE_NOTHROW(levels.diff.mToClient::send = [&]() {
              REQUIRE(levels.diff.blob().dump() == "{"
                "\"asks\":[{\"price\":1234.69,\"size\":0.11234566}],"
                "\"bids\":[{\"price\":1234.5},{\"price\":1234.4,\"size\":0.12345678}],"
                "\"diff\":true"
              "}");
            });
            REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
              REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":1234.5}");
            });
            REQUIRE_NOTHROW(levels.read_from_gw(mLevels(
              { mLevel(1234.40, 0.12345678), mLevel(1234.55, 0.01234567) },
              { mLevel(1234.60, 1.23456789), mLevel(1234.69, 0.11234566) }
            )));
            REQUIRE(levels.diff.hello().dump() == "[{"
              "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234566}],"
              "\"bids\":[{\"price\":1234.4,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
            "}]");
          }
        }
      }
    }

    GIVEN("mRecentTrades") {
      mRecentTrades recentTrades;
      WHEN("defaults") {
        REQUIRE_NOTHROW(recentTrades = mRecentTrades());
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
        mLastOrder order;
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
      Price fairValue = 0;
      mEwma ewma(fairValue);
      WHEN("defaults") {
        REQUIRE_FALSE(ewma.mgEwmaM);
      }
      WHEN("assigned") {
        vector<Price> fairHistory = { 268.05, 258.73, 239.82, 250.21, 224.49, 242.53, 248.25, 270.58, 252.77, 273.55,
                                      255.90, 226.10, 225.00, 263.12, 218.36, 254.73, 218.65, 252.40, 296.10, 222.20 };
        REQUIRE_NOTHROW(ewma.fairValue96h.mFromDb::push = ewma.mFromDb::push = [&]() {
          INFO("push()");
        });
        for (const Price &it : fairHistory) {
          REQUIRE_NOTHROW(fairValue = it);
          REQUIRE_NOTHROW(ewma.timer_60s(0));
        };
        REQUIRE_NOTHROW(qp.mediumEwmaPeriods = 20);
        REQUIRE_NOTHROW(qp._diffVLEP =
                        qp._diffLEP  =
                        qp._diffMEP  =
                        qp._diffSEP  =
                        qp._diffXSEP =
                        qp._diffUEP  = true);
        REQUIRE_NOTHROW(ewma.calcFromHistory());
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
      Option option;
      mProduct product(option);
      const Price minTick = 0.01;
      product.minTick = &minTick;
      mOrders orders(product);
      mMarketLevels levels(product, orders);
      const Price fairValue = 500;
      const double targetPositionAutoPercentage = 0;
      mWalletPosition wallet(product, orders, targetPositionAutoPercentage, fairValue);
      wallet.base = mWallet(1, 0, "BTC");
      wallet.quote = mWallet(1000, 0, "EUR");
      mBroker broker(product, orders, levels, wallet);
      WHEN("assigned") {
        vector<RandId> randIds;
        Clock time = Tstamp;
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.50, 0.12345678, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(orders.find(randIds.back())->time = time);
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.51, 0.12345679, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(orders.find(randIds.back())->time = time);
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Bid, 1234.52, 0.12345680, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(orders.find(randIds.back())->time = time);
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Ask, 1234.50, 0.12345678, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(orders.find(randIds.back())->time = time);
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Ask, 1234.51, 0.12345679, false)));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), "", Status::Working, 0, 0, 0)));
        REQUIRE_NOTHROW(orders.find(randIds.back())->time = time);
        REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
        REQUIRE_NOTHROW(orders.upsert(mOrder(randIds.back(), Side::Ask, 1234.52, 0.12345680, false)));
        THEN("held amount") {
          REQUIRE_NOTHROW(wallet.profits.mFromDb::push = [&]() {
            INFO("push()");
          });
          bool askForFees = false;
          mLastOrder order;
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
          REQUIRE(string::npos != orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[0] + "\",\"preferPostOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[1] + "\",\"preferPostOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[2] + "\",\"preferPostOnly\":true,\"price\":1234.52,\"quantity\":0.1234568,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[3] + "\",\"preferPostOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[4] + "\",\"preferPostOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
        }
      }
    }
  }
}

#endif