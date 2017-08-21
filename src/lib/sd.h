#ifndef K_SD_H_
#define K_SD_H_

namespace K {
  class SD {
    public:
      static void main(Local<Object> exports) {
        NODE_SET_METHOD(exports, "computeStdevs", SD::ComputeStdevs);
      };
      static double roundNearest(double value, double minTick) {
        return round(value / minTick) * minTick;
      };
      static double roundUp(double value, double minTick) {
        return ceil(value / minTick) * minTick;
      };
      static double roundDown(double value, double minTick) {
        return floor(value / minTick) * minTick;
      };
      static double roundSide(double oP, double minTick, mSide oS) {
        if (oS == mSide::Bid) return roundDown(oP, minTick);
        else if (oS == mSide::Ask) return roundUp(oP, minTick);
        else return roundNearest(oP, minTick);
      };
    private:
      static void ComputeStdevs(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Float64Array> seqA = args[0].As<Float64Array>();
        Local<Float64Array> seqB = args[1].As<Float64Array>();
        Local<Float64Array> seqC = args[2].As<Float64Array>();
        Local<Float64Array> seqD = args[3].As<Float64Array>();
        double factor = args[4]->NumberValue();
        double mean = 0;
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("fv"), Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqA->Buffer()->GetContents().Data()), seqA->Length(), factor, &mean)));
        o->Set(FN::v8S("fvMean"), Number::New(isolate, mean));
        o->Set(FN::v8S("tops"), Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqB->Buffer()->GetContents().Data()), seqB->Length(), factor, &mean)));
        o->Set(FN::v8S("topsMean"), Number::New(isolate, mean));
        o->Set(FN::v8S("bid"), Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqC->Buffer()->GetContents().Data()), seqC->Length(), factor, &mean)));
        o->Set(FN::v8S("bidMean"), Number::New(isolate, mean));
        o->Set(FN::v8S("ask"), Number::New(isolate, SD::ComputeStdev(reinterpret_cast<double*>(seqD->Buffer()->GetContents().Data()), seqD->Length(), factor, &mean)));
        o->Set(FN::v8S("askMean"), Number::New(isolate, mean));
        args.GetReturnValue().Set(o);
      };
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
