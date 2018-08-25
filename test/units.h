#ifndef K_UNITS_H_
#define K_UNITS_H_

namespace K {
  SCENARIO("general") {
    GIVEN("mConnectivity") {
      mConnectivity on, off;
      WHEN("assigned") {
        REQUIRE_NOTHROW(on = mConnectivity::Connected);
        REQUIRE_NOTHROW(off = mConnectivity::Disconnected);
        THEN("values") {
          REQUIRE_FALSE(!on);
          REQUIRE(!off);
          THEN("combined") {
            REQUIRE(on  * on  == on);
            REQUIRE(on  * off == off);
            REQUIRE(off * on  == off);
            REQUIRE(off * off == off);
          }
        }
      }
    }
  }

  SCENARIO("BTC/EUR") {
    GIVEN("mLevel") {
      mLevel level;
      WHEN("defaults") {
        REQUIRE_NOTHROW(level = mLevel());
        THEN("empty") {
          REQUIRE(level.empty());
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
          REQUIRE_FALSE(level.empty());
        }
        THEN("to json") {
          REQUIRE(((json)level).dump() == "{"
            "\"price\":1234.56,"
            "\"size\":0.12345678"
          "}");
        }
        WHEN("clear") {
          REQUIRE_NOTHROW(level.clear());
          THEN("values") {
            REQUIRE_FALSE(level.price);
            REQUIRE_FALSE(level.size);
          }
          THEN("empty") {
            REQUIRE(level.empty());
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
          REQUIRE(levels.empty());
        }
      }
      WHEN("assigned") {
        REQUIRE_NOTHROW(levels = mLevels(
          { mLevel(1234.56, 0.12345678) },
          { mLevel(1234.57, 0.12345679) }
        ));
        THEN("values") {
          REQUIRE(levels.spread() == Approx(0.01));
        }
        THEN("not empty") {
          REQUIRE_FALSE(levels.empty());
        }
        THEN("to json") {
          REQUIRE(((json)levels).dump() == "{"
            "\"asks\":[{\"price\":1234.57,\"size\":0.12345679}],"
            "\"bids\":[{\"price\":1234.56,\"size\":0.12345678}]"
          "}");
        }
        WHEN("clear") {
          REQUIRE_NOTHROW(levels.clear());
          THEN("values") {
            REQUIRE_FALSE(levels.spread());
          }
          THEN("empty") {
            REQUIRE(levels.empty());
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
      mProduct product;
      const mPrice  minTick = 0.01;
      const mAmount minSize = 0.001;
      product.minTick = &minTick;
      product.minSize = &minSize;
      unordered_map<mRandId, mOrder> orders;
      mMarketLevels levels(product, orders);
      WHEN("defaults") {
        THEN("fair value") {
          REQUIRE_FALSE(levels.fairValue);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            REQUIRE(levels.stats.fairPrice.blob().dump() == "{\"price\":0.0}");
          });
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
            INFO("refresh()");
          });
          REQUIRE_NOTHROW(levels.filter());
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
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
          INFO("refresh()");
        });
        REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::BBO);
        vector<mRandId> randIds;
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.52, 0.34567890, false));
        REQUIRE_NOTHROW(orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.52, 0.23456789, false));
        REQUIRE_NOTHROW(orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.55, 0.01234567, false));
        REQUIRE_NOTHROW(orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(orders[randIds.back()] = mOrder(randIds.back(), mSide::Ask, 1234.69, 0.01234568, false));
        REQUIRE_NOTHROW(orders[randIds.back()].status = mStatus::Working);
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
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
            FAIL("refresh() while filtering");
          });
          REQUIRE_NOTHROW(levels.filter());
          REQUIRE(levels.fairValue == 1234.55);
        }
        THEN("fair value weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::wBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            FAIL("send() while filtering");
          });
          REQUIRE_NOTHROW(levels.filter());
          REQUIRE(levels.fairValue == 1234.59);
        }
        THEN("fair value reversed weight") {
          REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::rwBBO);
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            FAIL("send() while filtering");
          });
          REQUIRE_NOTHROW(levels.filter());
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
        REQUIRE_NOTHROW(recentTrades.insert(0.01234566, 1234.57, mSide::Ask));
        REQUIRE_NOTHROW(recentTrades.insert(0.01234567, 1234.58, mSide::Ask));
        REQUIRE_NOTHROW(recentTrades.insert(0.12345678, 1234.56, mSide::Bid));
        REQUIRE_NOTHROW(recentTrades.insert(0.12345679, 1234.50, mSide::Bid));
        REQUIRE_NOTHROW(recentTrades.insert(0.12345678, 1234.60, mSide::Ask));
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

    GIVEN("mBroker") {
      mProduct product;
      const mPrice minTick = 0.01;
      product.minTick = &minTick;
      unordered_map<mRandId, mOrder> orders;
      mMarketLevels levels(product, orders);
      const mPrice fairValue = 0;
      const double targetPositionAutoPercentage = 0;
      mWalletPosition wallet(targetPositionAutoPercentage, fairValue);
      mBroker broker(product, wallet, levels);
      WHEN("assigned") {
        vector<mRandId> randIds;
        mClock time = Tstamp;
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.50, 0.12345678, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.51, 0.12345679, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Bid, 1234.52, 0.12345680, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Ask, 1234.50, 0.12345678, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Ask, 1234.51, 0.12345679, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Ask, 1234.52, 0.12345680, false));
        REQUIRE_NOTHROW(broker.orders[randIds.back()].status = mStatus::Working);
        REQUIRE_NOTHROW(broker.orders[randIds.back()].time = time);
        REQUIRE_NOTHROW(randIds.push_back(mRandom::uuid36Id()));
        REQUIRE_NOTHROW(broker.orders[randIds.back()] = mOrder(randIds.back(), mSide::Ask, 1234.52, 0.12345681, false));
        THEN("held amount") {
          REQUIRE(broker.calcHeldAmount(mSide::Bid) == Approx(457.22592546));
          REQUIRE(broker.calcHeldAmount(mSide::Ask) == 0.37037037);
        }
        THEN("to json") {
          REQUIRE(string::npos == broker.blob().dump().find("\"status\":0"));
          REQUIRE(string::npos == broker.blob().dump().find("\"status\":2"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[0] + "\",\"preferPostOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[1] + "\",\"preferPostOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[2] + "\",\"preferPostOnly\":true,\"price\":1234.52,\"quantity\":0.1234568,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[3] + "\",\"preferPostOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[4] + "\",\"preferPostOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
          REQUIRE(string::npos != broker.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":0,\"orderId\":\"" + randIds[5] + "\",\"preferPostOnly\":true,\"price\":1234.52,\"quantity\":0.1234568,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":2,\"type\":0}"));
        }
      }
    }
  }
}

#endif