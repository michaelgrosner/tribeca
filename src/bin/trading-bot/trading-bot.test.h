SCENARIO_METHOD(TradingBot, "NULL BTC/EUR") {
  gateway = Gw::new_Gw("NULL");
  gateway->exchange = "NULL";
  gateway->base     = "BTC";
  gateway->quote    = "EUR";
  gateway->handshake(true);
  GIVEN("MarketLevels") {
    WHEN("defaults") {
      THEN("fair value") {
        REQUIRE_FALSE(engine.levels.fairValue);
        REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = [&]() {
          REQUIRE(engine.levels.stats.fairPrice.blob().dump() == "{\"price\":0.0}");
        });
        REQUIRE_FALSE(engine.levels.ready());
        REQUIRE_FALSE(engine.levels.fairValue);
      }
    }
    WHEN("assigned") {
      for (Order *const it : engine.orders.working())
        engine.orders.purge(it);
      REQUIRE_NOTHROW(engine.levels.diff.read = [&]() {
        REQUIRE(engine.levels.diff.blob().dump() == "{"
          "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234569}],"
          "\"bids\":[{\"price\":1234.5,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
        "}");
      });
      REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = [&]() {
        REQUIRE(engine.levels.stats.fairPrice.blob().dump() == "{\"price\":1234.55}");
      });
      REQUIRE_NOTHROW(engine.qp.fvModel = tribeca::FairValueModel::BBO);
      vector<string> randIds;
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.52, 0.34567890, Tstamp, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.52, 0.23456789, Tstamp, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.55, 0.01234567, Tstamp, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Ask, 1234.69, 0.01234568, Tstamp, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, Tstamp, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(engine.levels.read_from_gw({
        { {1234.50, 0.12345678}, {1234.55, 0.01234567} },
        { {1234.60, 1.23456789}, {1234.69, 0.11234569} }
      }));
      THEN("filters") {
        REQUIRE(engine.levels.bids.size() == 1);
        REQUIRE(engine.levels.bids[0].price == 1234.50);
        REQUIRE(engine.levels.bids[0].size  == 0.12345678);
        REQUIRE(engine.levels.asks.size() == 2);
        REQUIRE(engine.levels.asks[0].price == 1234.60);
        REQUIRE(engine.levels.asks[0].size  == 1.23456789);
        REQUIRE(engine.levels.asks[1].price == 1234.69);
        REQUIRE(engine.levels.asks[1].size  == 0.10000001);
        REQUIRE(engine.levels.unfiltered.bids.size() == 2);
        REQUIRE(engine.levels.unfiltered.bids[0].price == 1234.50);
        REQUIRE(engine.levels.unfiltered.bids[0].size  == 0.12345678);
        REQUIRE(engine.levels.unfiltered.bids[1].price == 1234.55);
        REQUIRE(engine.levels.unfiltered.bids[1].size  == 0.01234567);
        REQUIRE(engine.levels.unfiltered.asks.size() == 2);
        REQUIRE(engine.levels.unfiltered.asks[0].price == 1234.60);
        REQUIRE(engine.levels.unfiltered.asks[0].size  == 1.23456789);
        REQUIRE(engine.levels.unfiltered.asks[1].price == 1234.69);
        REQUIRE(engine.levels.unfiltered.asks[1].size  == 0.11234569);
      }
      THEN("fair value") {
        REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = []() {
          FAIL("broadcast() while filtering");
        });
        REQUIRE(engine.levels.ready());
        REQUIRE(engine.levels.fairValue == 1234.55);
      }
      THEN("fair value weight") {
        REQUIRE_NOTHROW(engine.qp.fvModel = tribeca::FairValueModel::wBBO);
        REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = [&]() {
          FAIL("broadcast() while filtering");
        });
        REQUIRE(engine.levels.ready());
        REQUIRE(engine.levels.fairValue == 1234.59);
      }
      THEN("fair value reversed weight") {
        REQUIRE_NOTHROW(engine.qp.fvModel = tribeca::FairValueModel::rwBBO);
        REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = [&]() {
          FAIL("broadcast() while filtering");
        });
        REQUIRE(engine.levels.ready());
        REQUIRE(engine.levels.fairValue == 1234.51);
      }
      WHEN("diff") {
        REQUIRE(engine.levels.diff.hello().dump() == "[{"
          "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234569}],"
          "\"bids\":[{\"price\":1234.5,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
        "}]");
        REQUIRE_FALSE(engine.levels.diff.empty());
        THEN("broadcast") {
          REQUIRE_NOTHROW(engine.qp.delayUI = 0);
          this_thread::sleep_for(chrono::milliseconds(370));
          REQUIRE_NOTHROW(engine.levels.diff.read = [&]() {
            REQUIRE(engine.levels.diff.blob().dump() == "{"
              "\"asks\":[{\"price\":1234.69,\"size\":0.11234566}],"
              "\"bids\":[{\"price\":1234.5},{\"price\":1234.4,\"size\":0.12345678}],"
              "\"diff\":true"
            "}");
          });
          REQUIRE_NOTHROW(engine.levels.stats.fairPrice.read = [&]() {
            REQUIRE(engine.levels.stats.fairPrice.blob().dump() == "{\"price\":1234.5}");
          });
          REQUIRE_NOTHROW(engine.levels.read_from_gw({
            { {1234.40, 0.12345678}, {1234.55, 0.01234567} },
            { {1234.60, 1.23456789}, {1234.69, 0.11234566} }
            }));
          REQUIRE(engine.levels.diff.hello().dump() == "[{"
            "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234566}],"
            "\"bids\":[{\"price\":1234.4,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
          "}]");
        }
      }
    }
  }

  GIVEN("RecentTrades") {
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.lastBuyPrice = 0);
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.lastSellPrice = 0);
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.buys = {});
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.sells = {});
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.sumBuys = 0);
    REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.sumSells = 0);
    WHEN("defaults") {
      THEN("empty") {
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.lastBuyPrice);
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.lastSellPrice);
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.buys.size());
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.sells.size());
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumBuys);
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumSells);
      }
    }
    WHEN("assigned") {
      tribeca::LastOrder order = {};
      REQUIRE_NOTHROW(order.price = 1234.57);
      REQUIRE_NOTHROW(order.filled = 0.01234566);
      REQUIRE_NOTHROW(order.side = Side::Ask);
      REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.insert(order));
      REQUIRE_NOTHROW(order.price = 1234.58);
      REQUIRE_NOTHROW(order.filled = 0.01234567);
      REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.insert(order));
      REQUIRE_NOTHROW(order.price = 1234.56);
      REQUIRE_NOTHROW(order.filled = 0.12345678);
      REQUIRE_NOTHROW(order.side = Side::Bid);
      REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.insert(order));
      REQUIRE_NOTHROW(order.price = 1234.50);
      REQUIRE_NOTHROW(order.filled = 0.12345679);
      REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.insert(order));
      REQUIRE_NOTHROW(order.price = 1234.60);
      REQUIRE_NOTHROW(order.filled = 0.12345678);
      REQUIRE_NOTHROW(order.side = Side::Ask);
      REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.insert(order));
      THEN("values") {
        REQUIRE(engine.wallet.safety.recentTrades.lastBuyPrice == 1234.50);
        REQUIRE(engine.wallet.safety.recentTrades.lastSellPrice == 1234.60);
        REQUIRE(engine.wallet.safety.recentTrades.buys.size() == 2);
        REQUIRE(engine.wallet.safety.recentTrades.sells.size() == 3);
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumBuys);
        REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumSells);
        WHEN("reset") {
          THEN("skip") {
            REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.expire());
            REQUIRE(engine.wallet.safety.recentTrades.lastBuyPrice == 1234.50);
            REQUIRE(engine.wallet.safety.recentTrades.lastSellPrice == 1234.60);
            REQUIRE(engine.wallet.safety.recentTrades.buys.size() == 1);
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.sells.size());
            REQUIRE(engine.wallet.safety.recentTrades.sumBuys == 0.09876546);
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumSells);
          }
          THEN("expired") {
            this_thread::sleep_for(chrono::milliseconds(1001));
            REQUIRE_NOTHROW(engine.qp.tradeRateSeconds = 1);
            REQUIRE_NOTHROW(engine.wallet.safety.recentTrades.expire());
            REQUIRE(engine.wallet.safety.recentTrades.lastBuyPrice == 1234.50);
            REQUIRE(engine.wallet.safety.recentTrades.lastSellPrice == 1234.60);
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.buys.size());
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.sells.size());
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumBuys);
            REQUIRE_FALSE(engine.wallet.safety.recentTrades.sumSells);
          }
        }
      }
    }
  }

  GIVEN("Safety") {
    REQUIRE_NOTHROW(engine.wallet.safety.read = [&]() {
      INFO("read()");
    });

    WHEN("calcSizes") {
      REQUIRE_NOTHROW(Wallet::reset(1.0, 0, &engine.wallet.base));
      REQUIRE_NOTHROW(Wallet::reset(1000.0, 0, &engine.wallet.quote));
      REQUIRE_NOTHROW(engine.levels.fairValue = 500.0);
      REQUIRE_NOTHROW(engine.wallet.base.value = 3.0);
      REQUIRE_NOTHROW(engine.qp.percentageValues = true);
      REQUIRE_NOTHROW(engine.qp.sellSizePercentage = 10.0);
      REQUIRE_NOTHROW(engine.qp.buySizePercentage = 20.0);
      REQUIRE_NOTHROW(engine.wallet.target.positionDivergence = 1.0);

      WHEN("raw values") {
        REQUIRE_NOTHROW(engine.qp.percentageValues = false);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.01) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.02) == engine.wallet.safety.buySize);
      }

      THEN("pct total value") {
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::Value);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.6) == engine.wallet.safety.buySize);
      }

      THEN("pct side balance") {
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::Side);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.4) == engine.wallet.safety.buySize);
      }

      THEN("with 0\% tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 0.0);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.0) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.0) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1/3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.0) == engine.wallet.safety.buySize);
      }

      THEN("with low tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 0.5);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.15) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.05) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1/3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.0125/3) == engine.wallet.safety.buySize);
      }

      THEN("with matched tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 1.0);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.4) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.2) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.1/3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.2/3) == engine.wallet.safety.buySize);
      }

      THEN("with 50\% tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 1.5);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.3) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.6) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.06) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.36) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.012) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.216) == engine.wallet.safety.buySize);
      }

      THEN("with high tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 2.0);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.2) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.6) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.0) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.4) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.0) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.8/3) == engine.wallet.safety.buySize);
      }

      THEN("with 100\% tbp") {
        REQUIRE_NOTHROW(engine.wallet.target.targetBasePosition = 3.0);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPValue);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.0) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.6) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.orderPctTotal = tribeca::OrderPctTotal::TBPSide);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 1.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.0) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.4) == engine.wallet.safety.buySize);
        REQUIRE_NOTHROW(engine.qp.tradeSizeTBPExp = 2.0);
        REQUIRE_NOTHROW(engine.wallet.safety.calc());
        REQUIRE(Approx(0.0) == engine.wallet.safety.sellSize);
        REQUIRE(Approx(0.8/3) == engine.wallet.safety.buySize);
      }
    }
  }

  GIVEN("Ewma") {
    engine.levels.fairValue = 0;
    WHEN("defaults") {
      REQUIRE_FALSE(engine.levels.stats.ewma.mgEwmaM);
    }
    WHEN("assigned") {
      vector<Price> fairHistory = { 268.05, 258.73, 239.82, 250.21, 224.49, 242.53, 248.25, 270.58, 252.77, 273.55,
                                    255.90, 226.10, 225.00, 263.12, 218.36, 254.73, 218.65, 252.40, 296.10, 222.20 };
      REQUIRE_NOTHROW(engine.levels.stats.ewma.fairValue96h.Backup::push = engine.levels.stats.ewma.Backup::push = [&]() {
        INFO("push()");
      });
      for (const Price &it : fairHistory) {
        REQUIRE_NOTHROW(engine.levels.fairValue = it);
        REQUIRE_NOTHROW(engine.levels.stats.ewma.timer_60s(0));
      };
      REQUIRE_NOTHROW(engine.qp.mediumEwmaPeriods = 20);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 0);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 1);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 2);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 3);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 4);
      REQUIRE_NOTHROW(engine.qp._diffEwma |= true << 5);
      REQUIRE_NOTHROW(K.clicked(&engine.qp));
      THEN("values") {
        REQUIRE(engine.levels.stats.ewma.mgEwmaVL == Approx(266.1426832796));
        REQUIRE(engine.levels.stats.ewma.mgEwmaL == Approx(264.4045182289));
        REQUIRE(engine.levels.stats.ewma.mgEwmaM == Approx(261.3765619062));
        REQUIRE(engine.levels.stats.ewma.mgEwmaS == Approx(256.7706209412));
        REQUIRE(engine.levels.stats.ewma.mgEwmaXS == Approx(247.5567169778));
        REQUIRE(engine.levels.stats.ewma.mgEwmaU == Approx(245.5969655991));
        REQUIRE(engine.levels.stats.ewma.lifetime() == 24000000);
        THEN("to json") {
          REQUIRE(((json)engine.levels.stats.ewma).dump() == "{"
            "\"ewmaExtraShort\":247.55671697778087,"
            "\"ewmaLong\":264.40451822891674,"
            "\"ewmaMedium\":261.3765619061748,"
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

  GIVEN("Broker") {
    REQUIRE_NOTHROW(engine.qp.mode = tribeca::QuotingMode::Top);
    REQUIRE_NOTHROW(engine.qp.autoPositionMode = tribeca::AutoPositionMode::Manual);
    REQUIRE_NOTHROW(engine.qp.aggressivePositionRebalancing = tribeca::APR::Off);
    REQUIRE_NOTHROW(engine.qp.safety = tribeca::QuotingSafety::Off);
    REQUIRE_NOTHROW(engine.qp.protectionEwmaQuotePrice = false);
    REQUIRE_NOTHROW(engine.qp.widthPercentage = false);
    REQUIRE_NOTHROW(engine.qp.percentageValues = false);
    REQUIRE_NOTHROW(engine.qp.widthPing = 1);
    REQUIRE_NOTHROW(engine.qp.bestWidth = true);
    REQUIRE_NOTHROW(engine.qp.protectionEwmaWidthPing = false);
    REQUIRE_NOTHROW(engine.qp.targetBasePosition = 1);
    REQUIRE_NOTHROW(engine.qp.positionDivergence = 1);
    REQUIRE_NOTHROW(engine.qp.read = engine.levels.diff.read = engine.levels.stats.fairPrice.read = engine.wallet.read = engine.wallet.safety.read = engine.wallet.target.read = engine.broker.calculon.read = engine.broker.semaphore.read = [&]() {
      INFO("read()");
    });
    REQUIRE_NOTHROW(engine.qp.Backup::push = engine.wallet.target.Backup::push = engine.wallet.profits.Backup::push = [&]() {
      INFO("push()");
    });
    REQUIRE_NOTHROW(engine.broker.semaphore.read_from_gw(
      Connectivity::Disconnected
    ));
    REQUIRE_NOTHROW(engine.levels.fairValue = 500);
    REQUIRE_NOTHROW(engine.wallet.read_from_gw({
      {"BTC", 1,    0},
      {"EUR", 1000, 0}
    }));
    WHEN("assigned") {
      for (Order *const it : engine.orders.working())
        engine.orders.purge(it);
      vector<string> randIds;
      const Clock time = Tstamp;
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.50, 0.12345678, time-69, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.51, 0.12345679, time-69, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Bid, 1234.52, 0.12345680, time-69, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Ask, 1234.50, 0.12345678, time-69, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Ask, 1234.51, 0.12345679, time-69, false, randIds.back()}));
      REQUIRE_NOTHROW(engine.orders.upsert({(Side)0, 0, 0, time, false, randIds.back(), "", Status::Working, 0}));
      REQUIRE_NOTHROW(randIds.push_back(Random::uuid36Id()));
      REQUIRE_NOTHROW(engine.orders.upsert({Side::Ask, 1234.52, 0.12345680, time, false, randIds.back()}));
      THEN("held amount") {
        REQUIRE_NOTHROW(engine.orders.updated.price = 1);
        REQUIRE_NOTHROW(engine.orders.updated.side = Side::Ask);
        REQUIRE_NOTHROW(engine.wallet.calcFundsAfterOrder());
        REQUIRE_NOTHROW(engine.orders.updated.side = Side::Bid);
        REQUIRE_NOTHROW(engine.wallet.calcFundsAfterOrder());
        REQUIRE(engine.wallet.base.held == 0.37037037);
        REQUIRE(engine.wallet.quote.held == Approx(457.22592546));
      }
      THEN("to json") {
        REQUIRE(string::npos == engine.orders.blob().dump().find("\"status\":0"));
        REQUIRE(string::npos == engine.orders.blob().dump().find("\"status\":2"));
        REQUIRE(string::npos != engine.orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[0] + "\",\"postOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
        REQUIRE(string::npos != engine.orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[1] + "\",\"postOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
        REQUIRE(string::npos != engine.orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[2] + "\",\"postOnly\":true,\"price\":1234.52,\"quantity\":0.1234568,\"side\":0,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
        REQUIRE(string::npos != engine.orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[3] + "\",\"postOnly\":true,\"price\":1234.5,\"quantity\":0.12345678,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
        REQUIRE(string::npos != engine.orders.blob().dump().find("{\"exchangeId\":\"\",\"isPong\":false,\"latency\":69,\"orderId\":\"" + randIds[4] + "\",\"postOnly\":true,\"price\":1234.51,\"quantity\":0.12345679,\"side\":1,\"status\":1,\"time\":" + to_string(time) + ",\"timeInForce\":0,\"type\":0}"));
      }
    }
    WHEN("ready") {
      REQUIRE_NOTHROW(engine.levels.read_from_gw({
        { },
        { }
      }));
      REQUIRE_NOTHROW(engine.broker.semaphore.click({
        {"agree", 0}
      }));
      REQUIRE_NOTHROW(engine.broker.calculon.quotes.bid.clear(tribeca::QuoteState::MissingData));
      REQUIRE_NOTHROW(engine.broker.calculon.quotes.ask.clear(tribeca::QuoteState::MissingData));
      REQUIRE(engine.broker.calculon.quotes.bid.empty());
      REQUIRE(engine.broker.calculon.quotes.ask.empty());
      REQUIRE_FALSE(engine.broker.ready());
      REQUIRE(engine.broker.calculon.quotes.bid.state == tribeca::QuoteState::Disconnected);
      REQUIRE(engine.broker.calculon.quotes.ask.state == tribeca::QuoteState::Disconnected);
      REQUIRE_NOTHROW(engine.broker.purge());
      REQUIRE_NOTHROW(engine.broker.semaphore.read_from_gw(
        Connectivity::Connected
      ));
      REQUIRE(engine.broker.ready());
      REQUIRE_FALSE(engine.levels.ready());
      REQUIRE_NOTHROW(engine.levels.read_from_gw({ {
        {699, 0.12345678},
        {698, 0.12345678},
        {696, 0.12345678}
      }, {
        {701, 0.12345678},
        {702, 0.12345678},
        {704, 0.12345678}
      } }));
      REQUIRE(engine.levels.fairValue == 700);
      REQUIRE(engine.levels.ready());
      REQUIRE_NOTHROW(engine.wallet.read_from_gw({
        {"BTC", 0, 0},
        {"EUR", 0, 0}
      }));
      REQUIRE_FALSE(engine.wallet.ready());
      REQUIRE_NOTHROW(engine.wallet.read_from_gw({
        {"BTC", 1,    0},
        {"EUR", 1000, 0}
      }));
      REQUIRE_NOTHROW(engine.wallet.safety.timer_1s());
      REQUIRE(engine.wallet.ready());
      REQUIRE_NOTHROW(engine.broker.calcQuotes());
      REQUIRE(engine.broker.calculon.quotes.ask.empty());
      REQUIRE(engine.broker.calculon.quotes.bid.empty());
      THEN("agree") {
        REQUIRE(engine.broker.ready());
        REQUIRE_NOTHROW(engine.qp.click(engine.qp));
        REQUIRE_NOTHROW(engine.broker.semaphore.click({
          {"agree", 1}
        }));
        WHEN("quoting") {
          REQUIRE_NOTHROW(engine.broker.calculon.calcQuotes());
          REQUIRE_FALSE(engine.broker.calculon.quotes.bid.empty());
          REQUIRE_FALSE(engine.broker.calculon.quotes.ask.empty());
          THEN("to json") {
            REQUIRE(((json)engine.broker.calculon.quotes.bid).dump() == "{"
              "\"price\":699.01,"
              "\"size\":0.02"
            "}");
            REQUIRE(((json)engine.broker.calculon.quotes.ask).dump() == "{"
              "\"price\":700.99,"
              "\"size\":0.01"
            "}");
          }
          WHEN("widthPing=2") {
            REQUIRE_NOTHROW(engine.qp.widthPing = 2);
            REQUIRE_NOTHROW(engine.broker.calculon.calcQuotes());
            REQUIRE_FALSE(engine.broker.calculon.quotes.bid.empty());
            REQUIRE_FALSE(engine.broker.calculon.quotes.ask.empty());
            THEN("to json") {
              REQUIRE(((json)engine.broker.calculon.quotes.bid).dump() == "{"
                "\"price\":698.01,"
                "\"size\":0.02"
              "}");
              REQUIRE(((json)engine.broker.calculon.quotes.ask).dump() == "{"
                "\"price\":701.99,"
                "\"size\":0.01"
              "}");
            }
          }
          WHEN("widthPing=3,bestWidth=false") {
            REQUIRE_NOTHROW(engine.qp.bestWidth = false);
            REQUIRE_NOTHROW(engine.qp.widthPing = 3);
            REQUIRE_NOTHROW(engine.broker.calculon.calcQuotes());
            REQUIRE_FALSE(engine.broker.calculon.quotes.bid.empty());
            REQUIRE_FALSE(engine.broker.calculon.quotes.ask.empty());
            THEN("to json") {
              REQUIRE(((json)engine.broker.calculon.quotes.bid).dump() == "{"
                "\"price\":698.5,"
                "\"size\":0.02"
              "}");
              REQUIRE(((json)engine.broker.calculon.quotes.ask).dump() == "{"
                "\"price\":701.5,"
                "\"size\":0.01"
              "}");
            }
          }
          WHEN("widthPing=3") {
            REQUIRE_NOTHROW(engine.qp.widthPing = 3);
            REQUIRE_NOTHROW(engine.broker.calculon.calcQuotes());
            REQUIRE_FALSE(engine.broker.calculon.quotes.bid.empty());
            REQUIRE_FALSE(engine.broker.calculon.quotes.ask.empty());
            THEN("to json") {
              REQUIRE(((json)engine.broker.calculon.quotes.bid).dump() == "{"
                "\"price\":698.01,"
                "\"size\":0.02"
              "}");
              REQUIRE(((json)engine.broker.calculon.quotes.ask).dump() == "{"
                "\"price\":701.99,"
                "\"size\":0.01"
              "}");
            }
          }
          WHEN("widthPing=4") {
            REQUIRE_NOTHROW(engine.qp.widthPing = 4);
            REQUIRE_NOTHROW(engine.broker.calculon.calcQuotes());
            REQUIRE_FALSE(engine.broker.calculon.quotes.bid.empty());
            REQUIRE_FALSE(engine.broker.calculon.quotes.ask.empty());
            THEN("to json") {
              REQUIRE(((json)engine.broker.calculon.quotes.bid).dump() == "{"
                "\"price\":696.01,"
                "\"size\":0.02"
              "}");
              REQUIRE(((json)engine.broker.calculon.quotes.ask).dump() == "{"
                "\"price\":703.99,"
                "\"size\":0.01"
              "}");
            }
          }
        }
      }
    }
  }

  GIVEN("Issue #909 Corrupt Tradehistory Found") {
    engine.qp.click(R"({"aggressivePositionRebalancing":1,"aprMultiplier":3.0,"audio":false,"autoPositionMode":0,"bestWidth":true,"bestWidthSize":0.0,"bullets":2,"buySize":0.02,"buySizeMax":false,"buySizePercentage":1.0,"cancelOrdersAuto":false,"cleanPongsAuto":0.0,"delayUI":3,"ewmaSensiblityPercentage":0.5,"extraShortEwmaPeriods":12,"fvModel":0,"localBalance":true,"longEwmaPeriods":200,"mediumEwmaPeriods":100,"mode":0,"percentageValues":true,"pingAt":0,"pongAt":1,"positionDivergence":0.9,"positionDivergenceMin":0.4,"positionDivergenceMode":0,"positionDivergencePercentage":50.0,"positionDivergencePercentageMin":10.0,"profitHourInterval":72.0,"protectionEwmaPeriods":5,"protectionEwmaQuotePrice":false,"protectionEwmaWidthPing":false,"quotingEwmaTrendProtection":false,"quotingEwmaTrendThreshold":2.0,"quotingStdevBollingerBands":false,"quotingStdevProtection":0,"quotingStdevProtectionFactor":1.0,"quotingStdevProtectionPeriods":1200,"range":0.5,"rangePercentage":5.0,"safety":3,"sellSize":0.01,"sellSizeMax":false,"sellSizePercentage":1.0,"shortEwmaPeriods":50,"sopSizeMultiplier":2.0,"sopTradesMultiplier":2.0,"sopWidthMultiplier":2.0,"superTrades":0,"targetBasePosition":1.0,"targetBasePositionPercentage":50.0,"tradeRateSeconds":3,"tradesPerMinute":1,"ultraShortEwmaPeriods":3,"veryLongEwmaPeriods":400,"widthPercentage":false,"widthPing":0.01,"widthPingPercentage":0.25,"widthPong":0.01,"widthPongPercentage":0.25})"_json);
    REQUIRE_NOTHROW(engine.wallet.safety.trades.Backup::push = [&]() {
      INFO("push()");
    });
    REQUIRE_NOTHROW(engine.wallet.safety.trades.read = [&]() {
      INFO("read()");
    });
    auto parseTrade = [](string line)->tribeca::LastOrder {
      stringstream ss(line);
      string _, pingpong, side;
      tribeca::LastOrder order;
      ss >> _ >> _ >> _ >> _ >> pingpong >> _ >> side >> order.filled >> _ >> _ >> _ >> order.price;
      order.side = (side == "BUY" ? Side::Bid : Side::Ask);
      order.isPong = (pingpong == "PONG");
      return order;
    };
    vector<tribeca::LastOrder> loglines({
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
      for (const auto &order : loglines) {
        baseSign = (order.side == Side::Bid) ? 1 : -1;
        expectedBaseDelta += baseSign * order.filled;
        expectedQuoteDelta -= baseSign * order.filled * order.price;
        this_thread::sleep_for(chrono::milliseconds(2));
        engine.wallet.safety.trades.insert(order);
        Amount actualBaseDelta = 0;
        Amount actualQuoteDelta = 0;
        Amount expectedDiff = 0;
        Amount actualDiff = 0;
        for (const auto &trade : engine.wallet.safety.trades) {
          baseSign = (trade.side == Side::Bid) ? 1 : -1;
          actualBaseDelta += baseSign * (trade.quantity - trade.Kqty);
          Amount diff = baseSign * (trade.Kvalue - trade.value);
          actualQuoteDelta += diff;
          if (trade.Kdiff) {
            actualDiff += diff;
            expectedDiff += trade.Kdiff;
            REQUIRE(trade.Kdiff > 0);
          }
        }
        REQUIRE(abs(actualBaseDelta - expectedBaseDelta) < 0.000000000001);
        REQUIRE(abs(actualQuoteDelta - expectedQuoteDelta) < 0.000000000001);
        REQUIRE(abs(actualDiff - expectedDiff) < 0.000000000001);
      }
    }
  }
}
