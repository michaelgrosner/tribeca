#include "Krypto.ninja.h"
#include "hello-world.h"

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

int main(int argc, char** argv) {
  (args = &options)
    ->main(argc, argv)
    ->wait({
      &hello_world
    });
  return EXIT_FAILURE;
};
