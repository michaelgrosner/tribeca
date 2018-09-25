#include "Krypto.ninja.h"
#include "hello-world.h"

using namespace K;

int main(int argc, char** argv) {
  (args = &options)->main(argc, argv);
  hello_world(__FILE__);
  return EXIT_FAILURE;
};
