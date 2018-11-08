#include "Krypto.ninja.h"

          ;;;;;;;;;;;;;;;;;;;;;;
          ;;;;;    ;;;;    ;;;;;
          ;;;;;    ;;    ;;;;;;;
          ;;;;;        ;;;;;;;;;
          ;;;;;        ;;;;;;;;;
using namespace    K;    ;;;;;;;
          ;;;;;    ;;;;    ;;;;;
          ;;;;;    ;;;;    ;;;;;
          ;;;;;;;;;;;;;    ;;;;;
          ;;;;;;;;;;;;;;;;;;;;;;

#include "hello-world.h"

int main(int argc, char** argv) {
  options.main(argc, argv)->wait({
    &hello_world
  });
  return EXIT_FAILURE;
};
