#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  uWS::Hub hub(0, true);
  uv_check_t loop;
  uv_timer_t uiD_;
  Persistent<Function> noop;
  typedef Local<Value> (*uiCb)(Local<Value>);
  struct uiSess { map<string, Persistent<Function>> _cb; map<string, uiCb> cb; map<uiTXT, vector<Local<Object>>> D; int u = 0; };
  uWS::Group<uWS::SERVER> *uiGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
  int iOSR60 = 0;
  double uiMDT = 0;
  string uiNK64 = "";
  Persistent<Function> socket_;
  class UI: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
        o->InstanceTemplate()->SetInternalFieldCount(1);
        o->SetClassName(FN::v8S("UI"));
NODE_SET_PROTOTYPE_METHOD(o, "uiUp", _uiUp);
        socket_.Reset(isolate, o->GetFunction());
        exports->Set(FN::v8S("UI"), o->GetFunction());
        NODE_SET_METHOD(exports, "o60", UI::o60);
        NODE_SET_METHOD(exports, "uiLoop", UI::uiLoop);
        NODE_SET_METHOD(exports, "uiSnap", UI::_uiSnap);
        NODE_SET_METHOD(exports, "uiHand", UI::_uiHand);
        NODE_SET_METHOD(exports, "uiSend", UI::_uiSend);
        uiGroup->setUserData(new uiSess);
        if (uv_timer_init(uv_default_loop(), &uiD_)) {
          cout << "Errrror: UV uiD_ init timer failed." << endl;
          exit(1);
        }
        uiD_.data = (uiSess *) uiGroup->getUserData();
        if (uv_timer_start(&uiD_, uiD, 0, 0)) {
          cout << "Errrror: UV uiD_ start timer failed." << endl;
          exit(1);
        }
      }
      static void uiD(uv_timer_t *handle) {
        uiSess *sess = (uiSess*) handle->data;
        for (map<uiTXT, vector<Local<Object>>>::iterator it_=sess->D.begin(); it_!=sess->D.end(); ++it_)
          for (vector<Local<Object>>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
            uiUp(it_->first, *it);
        sess->D.clear();
      }
      static void uiSnap(uiTXT k, uiCb cb) {
        uiOn(uiBIT::SNAP, k, cb);
      }
      static void uiHand(uiTXT k, uiCb cb) {
        uiOn(uiBIT::MSG, k, cb);
      }
      static void uiSend(uiTXT k, Local<Object> o, bool h = false) {
        if (h) uiHold(k, o);
        else uiUp(k, o);
      }
      static void uiUp(uiTXT k, Local<Object> o) {
        Isolate* isolate = Isolate::GetCurrent();
        JSON Json;
        if (k == uiTXT::MarketData) {
          if (uiMDT+369 > chrono::milliseconds(chrono::seconds(std::time(NULL))).count()) return;
          uiMDT = chrono::milliseconds(chrono::seconds(std::time(NULL))).count();
        }
        MaybeLocal<String> v = o->IsUndefined() ? FN::v8S("") : Json.Stringify(isolate->GetCurrentContext(), shrinkHand(k, o));
        string m = string(1, (char)uiBIT::MSG).append(string(1, (char)k)).append(*String::Utf8Value(v.ToLocalChecked()));
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      static void uiOn(uiBIT k_, uiTXT _k, uiCb cb) {
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        string k = string(1, (char)k_).append(string(1, (char)_k));
        if (sess->cb.find(k) != sess->cb.end()) { cout << "Use only a single unique message handler for each \"" << k << "\" event" << endl; exit(1); }
        sess->cb[k] = cb;
      }
    protected:
      int port;
      string name;
      string key;
    private:
      explicit UI(int p_, string n_, string k_): port(p_), name(n_), key(k_) {
        Isolate* isolate = Isolate::GetCurrent();
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (name != "NULL" && key != "NULL" && name.length() > 0 && key.length() > 0) {
          B64::Encode(name.append(":").append(key), &uiNK64);
          uiNK64 = string("Basic ").append(uiNK64);
        }
        uiGroup->onConnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          sess->u++;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(sess->u) << " UI currently connected, last connection was from " << address.address << endl;
        });
        uiGroup->onDisconnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          sess->u--;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(sess->u) << " UI currently connected, last disconnection was from " << address.address << endl;
        });
        uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          string auth = req.getHeader("authorization").toString();
          typename uWS::WebSocket<uWS::SERVER>::Address address = res->getHttpSocket()->getAddress();
          if (uiNK64 != "" && auth == "") {
            cout << "UI authorization attempt from " << address.address << endl;
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth != uiNK64) {
            cout << "UI authorization failed from " << address.address << endl;
            document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url;
            stringstream content;
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            const string leaf = path.substr(path.find_last_of('.')+1);
            if (leaf == "/") {
              cout << "UI authorization success from " << address.address << endl;
              document.append("Content-Type: text/html; charset=UTF-8\r\n");
              url = "/index.html";
            } else if (leaf == "js") {
              document.append("Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n");
              url = path;
            } else if (leaf == "css") {
              document.append("Content-Type: text/css; charset=UTF-8\r\n");
              url = path;
            } else if (leaf == "png") {
              document.append("Content-Type: image/png\r\n");
              url = path;
            } else if (leaf == "mp3") {
              document.append("Content-Type: audio/mpeg\r\n");
              url = path;
            }
            if (url.length() > 0) {
              content << ifstream (string("app/pub").append(url)).rdbuf();
            } else {
              document = "HTTP/1.1 404 Not Found\r\n";
              content << "Today, is a beautiful day.";
            }
            document.append("Content-Length: ").append(to_string(content.str().length())).append("\r\n\r\n").append(content.str());
            res->write(document.data(), document.length());
          }
        });
        uiGroup->onMessage([isolate, sess](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          if (length > 1 && (sess->cb.find(string(message).substr(0,2)) != sess->cb.end())) {
            JSON Json;
            HandleScope hs(isolate);
            string m = string(message).substr(2, length-2);
            MaybeLocal<Value> v = (length > 2 && (m[0] == '[' || m[0] == '{')) ? Json.Parse(isolate->GetCurrentContext(), FN::v8S(m.data())) : FN::v8S(length > 2 ? m.data() : "");
            Local<Value> reply = (*sess->cb[string(message).substr(0,2)])(uiBIT::SNAP == (uiBIT)message[0] ? (Local<Value>)Undefined(isolate) : ((m == "true" || m == "false") ? (Local<Value>)Boolean::New(isolate, m == "true") : (v.IsEmpty() ? (Local<Value>)String::Empty(isolate) : v.ToLocalChecked())));
            if (!reply->IsUndefined() && uiBIT::SNAP == (uiBIT)message[0])
              webSocket->send(string(message).substr(0,2).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), shrinkSnap((uiTXT)message[1], reply->ToObject())).ToLocalChecked())).data(), uWS::OpCode::TEXT);
          }
          if (length > 1 && (sess->_cb.find(string(message).substr(0,2)) != sess->_cb.end())) {
            JSON Json;
            HandleScope hs(isolate);
            string m = string(message).substr(2, length-2);
            MaybeLocal<Value> v = (length > 2 && (m[0] == '[' || m[0] == '{')) ? Json.Parse(isolate->GetCurrentContext(), FN::v8S(m.data())) : FN::v8S(length > 2 ? m.data() : "");
            Local<Value> argv[] = {uiBIT::SNAP == (uiBIT)message[0] ? (Local<Value>)Undefined(isolate) : ((m == "true" || m == "false") ? (Local<Value>)Boolean::New(isolate, m == "true") : (v.IsEmpty() ? (Local<Value>)String::Empty(isolate) : v.ToLocalChecked()))};
            Local<Value> reply = Local<Function>::New(isolate, sess->_cb[string(message).substr(0,2)])->Call(isolate->GetCurrentContext()->Global(), 1, argv);
            if (!reply->IsUndefined() && uiBIT::SNAP == (uiBIT)message[0])
              webSocket->send(string(message).substr(0,2).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), shrinkSnap((uiTXT)message[1], reply->ToObject())).ToLocalChecked())).data(), uWS::OpCode::TEXT);
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if ((access("dist/sslcert/server.crt", F_OK) != -1) && (access("dist/sslcert/server.key", F_OK) != -1) && hub.listen(port, c, 0, uiGroup))
          cout << "UI ready over " << "HTTPS" << " on external port " << to_string(port) << endl;
        else if (hub.listen(port, nullptr, 0, uiGroup))
          cout << "UI ready over " << "HTTP" << " on external port " << to_string(port) << endl;
        else isolate->ThrowException(Exception::TypeError(FN::v8S(string("Use another UI port number, ").append(to_string(port)).append(" seems already in use").data())));
      }
      ~UI() {
        delete uiGroup;
      }
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(FN::v8S("Use the 'new' operator to create new UI objects")));
        UI* ui = new UI(args[0]->NumberValue(), FN::S8v(args[1]->ToString()), FN::S8v(args[2]->ToString()));
        ui->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
      static Local<Object> shrinkSnap(uiTXT k, Local<Object> snap) {
        Isolate* isolate = Isolate::GetCurrent();
        if (k == uiTXT::OrderStatusReports) return shrinkHand(k, snap);
        else if (k == uiTXT::MarketData) {
          MaybeLocal<Array> maybe_props = snap->GetOwnPropertyNames(Context::New(isolate));
          if (!maybe_props.IsEmpty()) {
            Local<Array> props = maybe_props.ToLocalChecked();
            Local<Array> snap_ = Array::New(isolate);
            for(uint32_t i=0; i < props->Length(); i++)
              snap_->Set(0, shrinkHand(k, snap->Get(props->Get(i))->ToObject()));
            return snap_->ToObject();
          }
        }
        return snap;
      }
      static Local<Object> shrinkHand(uiTXT k, Local<Object> snap) {
        Isolate* isolate = Isolate::GetCurrent();
        if (k == uiTXT::MarketData) {
          MaybeLocal<Array> maybe_props = snap->GetOwnPropertyNames(Context::New(isolate));
          if (!maybe_props.IsEmpty()) {
            Local<Array> snap_ = Array::New(isolate);
            snap_->Set(0, Array::New(isolate));
            snap_->Set(1, Array::New(isolate));
            Local<Array> props = maybe_props.ToLocalChecked();
            for(uint32_t i=0; i < props->Length(); i++) {
              Local<Object> lvl = snap->Get(props->Get(i))->ToObject();
              MaybeLocal<Array> maybe_props = lvl->GetOwnPropertyNames(Context::New(isolate));
              if (!maybe_props.IsEmpty()) {
                int lvls = 0;
                int side = FN::S8v(props->Get(i)->ToString()) == "bids" ? 0 : 1;
                Local<Array> props = maybe_props.ToLocalChecked();
                for(uint32_t i=0; i < props->Length(); i++) {
                  Local<Object> px = lvl->Get(props->Get(i))->ToObject();
                  MaybeLocal<Array> maybe_props = px->GetOwnPropertyNames(Context::New(isolate));
                  if (!maybe_props.IsEmpty()) {
                    Local<Array> props = maybe_props.ToLocalChecked();
                    for(uint32_t i=0; i < props->Length(); i++)
                      snap_->Get(side)->ToObject()->Set(lvls++, px->Get(props->Get(i))->ToNumber());
                  }
                }
              }
            }
            return snap_->ToObject();
          }
        } else if (k == uiTXT::OrderStatusReports) {
          MaybeLocal<Array> maybe_props = snap->GetOwnPropertyNames(Context::New(isolate));
          if (!maybe_props.IsEmpty()) {
            Local<Object> snap_ = Array::New(isolate);
            Local<Array> props = maybe_props.ToLocalChecked();
            for(uint32_t i=0; i < props->Length(); i++) {
              Local<Object> o = snap->Get(props->Get(i))->ToObject();
              MaybeLocal<Array> maybe_props = o->GetOwnPropertyNames(Context::New(isolate));
              Local<Array> props = maybe_props.ToLocalChecked();
              if (!maybe_props.IsEmpty()) {
                snap_->Set(i, Array::New(isolate));
                Local<Number> osr = o->Get(FN::v8S("orderStatus"))->ToNumber();
                snap_->Get(i)->ToObject()->Set(0, o->Get(FN::v8S("orderId"))->ToString());
                snap_->Get(i)->ToObject()->Set(1, osr);
                snap_->Get(i)->ToObject()->Set(2, o->Get(FN::v8S("side"))->ToNumber());
                snap_->Get(i)->ToObject()->Set(3, o->Get(FN::v8S("price"))->ToNumber());
                snap_->Get(i)->ToObject()->Set(4, o->Get(FN::v8S("quantity"))->ToNumber());
                snap_->Get(i)->ToObject()->Set(5, o->Get(FN::v8S("time"))->ToNumber());
                if ((mORS)osr->NumberValue() <= mORS::Working) {
                  snap_->Get(i)->ToObject()->Set(6, o->Get(FN::v8S("exchange"))->ToNumber());
                  snap_->Get(i)->ToObject()->Set(7, o->Get(FN::v8S("type"))->ToNumber());
                  snap_->Get(i)->ToObject()->Set(8, o->Get(FN::v8S("timeInForce"))->ToNumber());
                  snap_->Get(i)->ToObject()->Set(9, o->Get(FN::v8S("computationalLatency"))->ToNumber());
                  snap_->Get(i)->ToObject()->Set(10, o->Get(FN::v8S("leavesQuantity"))->ToNumber());
                  snap_->Get(i)->ToObject()->Set(11, o->Get(FN::v8S("isPong"))->ToNumber());
                }
              }
            }
            return snap_->ToObject();
          }
        }
        return snap;
      }
      static void _uiSnap(const FunctionCallbackInfo<Value>& args) {
        _uiOn(args, uiBIT::SNAP);
      }
      static void _uiHand(const FunctionCallbackInfo<Value>& args) {
        _uiOn(args, uiBIT::MSG);
      }
      static void _uiOn(const FunctionCallbackInfo<Value>& args, uiBIT k_) {
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(1, (char)k_).append(*String::Utf8Value(args[0]->ToString()));
        if (sess->_cb.find(k) != sess->_cb.end())
          return (void)isolate->ThrowException(Exception::TypeError(FN::v8S("Use only a single unique message handler for each different topic")));
        Persistent<Function> *_cb = &sess->_cb[k];
        _cb->Reset(isolate, Local<Function>::Cast(args[1]));
      }
      static void _uiUp(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        JSON Json;
        string k = FN::S8v(args[0]->ToString());
        if ((uiTXT)k[0] == uiTXT::MarketData) {
          if (uiMDT+369 > chrono::milliseconds(chrono::seconds(std::time(NULL))).count()) return;
          uiMDT = chrono::milliseconds(chrono::seconds(std::time(NULL))).count();
        }
        MaybeLocal<String> v = args[1]->IsUndefined() ? FN::v8S("") : Json.Stringify(isolate->GetCurrentContext(), shrinkHand((uiTXT)k[0], args[1]->ToObject()));
        string m = string(1, (char)uiBIT::MSG).append(k).append(*String::Utf8Value(v.ToLocalChecked()));
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      static void o60(const FunctionCallbackInfo<Value> &args) {
        Isolate *isolate = args.GetIsolate();
        if (args[0]->IsUndefined() ? false : args[0]->BooleanValue()) iOSR60 = 0;
        args.GetReturnValue().Set(Number::New(isolate, iOSR60));
      }
      static void _uiSend(const FunctionCallbackInfo<Value> &args) {
        if (args[2]->IsUndefined() ? false : args[2]->BooleanValue()) uiHold((uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->ToObject());
        else _uiUp(args);
      }
      static void uiHold(uiTXT k, Local<Object> o) {
        Isolate* isolate = Isolate::GetCurrent();
        bool isOSR = k == uiTXT::OrderStatusReports;
        if (isOSR && mORS::New == (mORS)o->Get(FN::v8S("orderStatus"))->NumberValue()) return (void)++iOSR60;
        Local<Object> qp_ = Local<Object>::New(isolate, qpRepo);
        if (!qp_->Get(FN::v8S("delayUI"))->NumberValue()) return uiUp(k, o);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (!isOSR) {
          for (map<uiTXT, vector<Local<Object>>>::iterator it=sess->D.begin(); it!=sess->D.end(); ++it)
            if (it->first == k) sess->D.erase(it);
        } else for (vector<Local<Object>>::iterator it = sess->D[k].begin(); it != sess->D[k].end(); ++it)
          if ((*it)->Get(FN::v8S("orderId"))->ToString() == o->Get(FN::v8S("orderId"))->ToString()) sess->D[k].erase(it);
        sess->D[k].push_back(o);
      }
      static void uiLoop(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        noop.Reset(isolate, Local<Function>::Cast(args[0]));
        uv_check_init((uv_loop_t *) hub.getLoop(), &loop);
        loop.data = isolate;
        uv_check_start(&loop, [](uv_check_t *loop) {
          Isolate *isolate = (Isolate *) loop->data;
          HandleScope hs(isolate);
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, noop), 0, nullptr);
        });
        uv_unref((uv_handle_t *) &loop);
      }
  };
}

#endif
