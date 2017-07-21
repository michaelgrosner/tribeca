#ifndef K_SD_H_
#define K_SD_H_

namespace K {
  class SD {
    public:
      static void main(Local<Object> exports) {
        NODE_SET_METHOD(exports, "computeStdevs", SD::ComputeStdevs);
      };
      static void ComputeStdevs(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Float64Array> seqA = args[0].As<Float64Array>();
        Local<Float64Array> seqB = args[1].As<Float64Array>();
        Local<Float64Array> seqC = args[2].As<Float64Array>();
        Local<Float64Array> seqD = args[3].As<Float64Array>();
        double factor = args[4]->NumberValue();
        double mean = 0;
        Local<Object> obj = Object::New(isolate);
        obj->Set(FN::v8S("fv"),       Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqA->Buffer()->GetContents().Data()), seqA->Length(), factor, &mean)));
        obj->Set(FN::v8S("fvMean"),   Number::New(isolate, mean));
        obj->Set(FN::v8S("tops"),     Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqB->Buffer()->GetContents().Data()), seqB->Length(), factor, &mean)));
        obj->Set(FN::v8S("topsMean"), Number::New(isolate, mean));
        obj->Set(FN::v8S("bid"),      Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqC->Buffer()->GetContents().Data()), seqC->Length(), factor, &mean)));
        obj->Set(FN::v8S("bidMean"),  Number::New(isolate, mean));
        obj->Set(FN::v8S("ask"),      Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqD->Buffer()->GetContents().Data()), seqD->Length(), factor, &mean)));
        obj->Set(FN::v8S("askMean"),  Number::New(isolate, mean));
        args.GetReturnValue().Set(obj);
      };
    private:
      static double ComputeStdev(double a[], int n, double f, double *mean) {
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
      };
  };
}

#endif
