#ifndef K_UI_H_
#define K_UI_H_

#include <nan.h>

namespace K {
  using Nan::New;

  class UI {
    public:
      static NAN_MODULE_INIT(main);
  };
}

#endif
