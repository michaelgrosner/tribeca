#ifndef K_STDEV_H_
#define K_STDEV_H_

#include <nan.h>
#include <math.h>

namespace K {
  using Nan::New;
  using Nan::Export;
  using v8::Local;
  using v8::Object;
  using v8::Float64Array;

  class Stdev {
    public:
      static NAN_MODULE_INIT(Init);
      static NAN_METHOD(ComputeStdevs);
    private:
      static double ComputeStdev(double[], int, double, double *);
  };
}

#endif
