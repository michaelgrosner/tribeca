#include "stdev.h"

namespace ctubio {
  void Stdev::Init(Local<Object> target) {
    Nan::Export(target, "computeStdevs", Stdev::ComputeStdevs);
  }

  NAN_METHOD(Stdev::ComputeStdevs) {
    v8::Local<v8::Float64Array> seqA = info[0].As<v8::Float64Array>();
    v8::Local<v8::Float64Array> seqB = info[1].As<v8::Float64Array>();
    v8::Local<v8::Float64Array> seqC = info[2].As<v8::Float64Array>();
    v8::Local<v8::Float64Array> seqD = info[3].As<v8::Float64Array>();

    double factor = info[4]->NumberValue();
    double minTick = info[5]->NumberValue();

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    obj->Set(Nan::New("fv").ToLocalChecked(), Nan::New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqA->Buffer()->GetContents().Data()), seqA->Length(), factor, minTick)));
    obj->Set(Nan::New("tops").ToLocalChecked(), Nan::New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqB->Buffer()->GetContents().Data()), seqB->Length(), factor, minTick)));
    obj->Set(Nan::New("bid").ToLocalChecked(), Nan::New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqC->Buffer()->GetContents().Data()), seqC->Length(), factor, minTick)));
    obj->Set(Nan::New("ask").ToLocalChecked(), Nan::New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqD->Buffer()->GetContents().Data()), seqD->Length(), factor, minTick)));

    info.GetReturnValue().Set(obj);
  }

  double Stdev::ComputeStdev(double a[], int n, double f, double m) {
    if(n == 0)
        return 0.0;
    double sum = 0;
    for(int i = 0; i < n; ++i)
       sum += a[i];
    double mean = sum / n;
    double sq_diff_sum = 0;
    for(int i = 0; i < n; ++i) {
       double diff = a[i] - mean;
       sq_diff_sum += diff * diff;
    }
    double variance = sq_diff_sum / n;
    return round((sqrt(variance) * f) / m) * m;
  }
}