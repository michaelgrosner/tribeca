#ifndef K_UNITS_H_
#define K_UNITS_H_

namespace K {
  TEST_CASE("mMarketLevels") {
    SECTION("mLevel") {
      SECTION("defaults") {
        REQUIRE_NOTHROW(mLevel());
      }
      SECTION("assigned") {
        mLevel level = mLevel(
          1234.56,
          0.12345678
        );
        SECTION("values") {
          REQUIRE(level.price == 1234.56);
          REQUIRE(level.size == 0.12345678);
        }
        SECTION("not empty") {
          REQUIRE_FALSE(level.empty());
        }
        SECTION("to json") {
          REQUIRE(((json)level).dump() == "{\"price\":1234.56,\"size\":0.12345678}");
        }
        SECTION("clear") {
          REQUIRE_NOTHROW(level.clear());
          SECTION("values") {
            REQUIRE_FALSE(level.price);
            REQUIRE_FALSE(level.size);
          }
          SECTION("empty") {
            REQUIRE(level.empty());
          }
          SECTION("to json") {
            REQUIRE(((json)level).dump() == "{\"price\":0.0}");
          }
        }
      }
    }
    SECTION("mLevels") {
      SECTION("defaults") {
        REQUIRE_NOTHROW(mLevels());
      }
      SECTION("assigned") {
        mLevels levels = mLevels(
          { mLevel(1234.56, 0.12345678) },
          { mLevel(1234.57, 0.12345678) }
        );
        SECTION("values") {
          REQUIRE(levels.spread() == Approx(0.01));
        }
        SECTION("not empty") {
          REQUIRE_FALSE(levels.empty());
        }
        SECTION("to json") {
          REQUIRE(((json)levels).dump() == "{\"asks\":[{\"price\":1234.57,\"size\":0.12345678}],\"bids\":[{\"price\":1234.56,\"size\":0.12345678}]}");
        }
        SECTION("clear") {
          REQUIRE_NOTHROW(levels.clear());
          SECTION("values") {
            REQUIRE_FALSE(levels.spread());
          }
          SECTION("empty") {
            REQUIRE(levels.empty());
          }
          SECTION("to json") {
            REQUIRE(((json)levels).dump() == "{\"asks\":[],\"bids\":[]}");
          }
        }
      }
    }
  }
}

#endif