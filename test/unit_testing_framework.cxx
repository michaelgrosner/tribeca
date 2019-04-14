#define CATCH_CONFIG_RUNNER
#include <catch.h>

void catch_exit(const int code) {
  const char *argv[] = {"K", "--durations yes", nullptr};
  const int argc = sizeof(argv) / sizeof(char*) - 1;
  exit(code ?: Catch::Session().run(argc, argv));
};
