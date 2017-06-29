#include "stdev.h"

namespace K {
  NAN_MODULE_INIT(Stdev::Init) {
    Export(target, "computeStdevs", Stdev::ComputeStdevs);
  }

  NAN_METHOD(Stdev::ComputeStdevs) {
    Local<Float64Array> seqA = info[0].As<Float64Array>();
    Local<Float64Array> seqB = info[1].As<Float64Array>();
    Local<Float64Array> seqC = info[2].As<Float64Array>();
    Local<Float64Array> seqD = info[3].As<Float64Array>();
    double factor = info[4]->NumberValue();
    double mean;

    Local<Object> obj = New<Object>();
    obj->Set(New("fv").ToLocalChecked(), New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqA->Buffer()->GetContents().Data()), seqA->Length(), factor, &mean)));
    obj->Set(New("fvMean").ToLocalChecked(), New(mean));
    obj->Set(New("tops").ToLocalChecked(), New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqB->Buffer()->GetContents().Data()), seqB->Length(), factor, &mean)));
    obj->Set(New("topsMean").ToLocalChecked(), New(mean));
    obj->Set(New("bid").ToLocalChecked(), New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqC->Buffer()->GetContents().Data()), seqC->Length(), factor, &mean)));
    obj->Set(New("bidMean").ToLocalChecked(), New(mean));
    obj->Set(New("ask").ToLocalChecked(), New(Stdev::ComputeStdev(reinterpret_cast<double*>(seqD->Buffer()->GetContents().Data()), seqD->Length(), factor, &mean)));
    obj->Set(New("askMean").ToLocalChecked(), New(mean));

    info.GetReturnValue().Set(obj);
  }

  double Stdev::ComputeStdev(double a[], int n, double f, double *mean) {
    if (n == 0) return 0.0;
    double sum = 0;
    for (int i = 0; i < n; ++i) sum += a[i];
    *mean = sum / n;
    double sq_diff_sum = 0;
    for (int i = 0; i < n; ++i) {
      double diff = a[i] - *mean;
      sq_diff_sum += diff * diff;
    }
    double variance = sq_diff_sum / n;
    return sqrt(variance) * f;
  }
}
