#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG {
    public:
      static void main(Local<Object> exports) {
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &gwCancelAll_)) { cout << FN::uiT() << "Errrror: GW gwCancelAll_ init timer failed." << endl; exit(1); }
          gwCancelAll_.data = gw;
          if (uv_timer_start(&gwCancelAll_, [](uv_timer_t *handle) {
            Gw* gw = (Gw*) handle->data;
            Isolate* isolate = Isolate::GetCurrent();
            HandleScope scope(isolate);
            Local<Object> o = Local<Object>::New(isolate, qpRepo);
            if (o->Get(FN::v8S("cancelOrdersAuto"))->BooleanValue())
              gw->cancelAll();
          }, 0, 300000)) { cout << FN::uiT() << "Errrror: GW gwCancelAll_ start timer failed." << endl; exit(1); }
        }).detach();
      }
  };
}

#endif
