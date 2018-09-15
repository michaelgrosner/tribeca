#include "Krypto.ninja.h"

int main(int argc, char** argv) {
  K::endingMsg = "Hello, World!";
  cout << __FILE__ << endl;
  raise(SIGINT);
  return EXIT_FAILURE;
};
