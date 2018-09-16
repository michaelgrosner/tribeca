#ifndef K_HELLO_WORLD_H_
#define K_HELLO_WORLD_H_

namespace K {
  const string greeting() {
    cout << "Hello, World!" << endl;
    return "OK";
  };

  void hello_world(const string &file) {
    epilogue = file + " executed " + greeting() + '.';
    raise(SIGINT);
  };
}

#endif
