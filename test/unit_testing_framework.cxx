#define CATCH_CONFIG_RUNNER
#include <catch.h>

void catch_exit(const int code) {
  exit(code ?: Catch::Session().run());
};
