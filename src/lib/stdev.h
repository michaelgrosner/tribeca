#ifndef CTUBIO_STDEV_H_
#define CTUBIO_STDEV_H_

#include <nan.h>
#include <math.h>

namespace ctubio {
  using v8::Local;
  using v8::Value;
  using v8::Number;
  using v8::Object;
  using v8::Float32Array;

  class Stdev {
    public:
      static void Init(v8::Local<v8::Object> target);
      static NAN_METHOD(ComputeStdevs);
    private:
      static double ComputeStdev(double[], int, double, double);
  };
}

#endif