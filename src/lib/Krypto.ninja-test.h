//! \file
//! \brief Unit testing framework and general test untis.
//! \note  Test units about benchmarks are removed after a while,
//!        but if you need some examples, see https://github.com/catchorg/Catch2/blob/master/projects/SelfTest/UsageTests/Benchmark.tests.cpp

#include <catch.h>

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
          REQUIRE_FALSE(((bool)off and (bool)off));
        }
      }
    }
  }
  GIVEN("Level") {
    Level level;
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
  GIVEN("Levels") {
    Levels levels;
    WHEN("defaults") {
      REQUIRE_NOTHROW(levels = Levels());
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
