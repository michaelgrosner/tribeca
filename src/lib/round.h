#ifndef CTUBIO_ROUND_H_
#define CTUBIO_ROUND_H_

#include <nan.h>
#include <math.h>

namespace ctubio {
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