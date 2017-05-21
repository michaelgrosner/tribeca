#ifndef ROUND_H
#define ROUND_H

#include <math.h>

#include <nan.h>
using v8::Local;
using v8::Object;


class Round {
  public:
    static void Init(v8::Local<v8::Object> target);
    static NAN_METHOD(RoundUp);
    static NAN_METHOD(RoundDown);
    static NAN_METHOD(RoundNearest);
    static NAN_METHOD(RoundSide);
  private:
    static bool Valid(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

#endif