#ifndef TRIBECA_ROUND_
#define TRIBECA_ROUND_

#include <nan.h>
#include <math.h>

namespace tribeca {
  using v8::Local;
  using v8::Value;
  using v8::Number;
  using v8::Object;

  class Round {
    public:
      static void Init(v8::Local<v8::Object> target);
      static NAN_METHOD(RoundUp);
      static NAN_METHOD(RoundDown);
      static NAN_METHOD(RoundNearest);
      static NAN_METHOD(RoundSide);
  };
}

#endif