#define CATCH_CONFIG_RUNNER
#include <catch.h>

static void catch_exit(const int code) {
  exit(code ?: Catch::Session().run());
};
