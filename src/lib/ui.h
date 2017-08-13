#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  class UI {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        CF::internal(exports);
        int port = stoi(CF::cfString("WebClientListenPort"));
        string name = CF::cfString("WebClientUsername");
        string key = CF::cfString("WebClientPassword");
        uiGroup->setUserData(new uiSess);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (name != "NULL" && key != "NULL" && name.length() > 0 && key.length() > 0) {
          B64::Encode(name.append(":").append(key), &uiNK64);
          uiNK64 = string("Basic ").append(uiNK64);
        }
        uiGroup->onConnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          sess->u++;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << FN::uiT() << to_string(sess->u) << " UI currently connected, last connection was from " << address.address << endl;
        });
        uiGroup->onDisconnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          sess->u--;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << FN::uiT() << to_string(sess->u) << " UI currently connected, last disconnection was from " << address.address << endl;
        });
        uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          string auth = req.getHeader("authorization").toString();
          typename uWS::WebSocket<uWS::SERVER>::Address address = res->getHttpSocket()->getAddress();
          if (uiNK64 != "" && auth == "") {
            cout << FN::uiT() << "UI authorization attempt from " << address.address << endl;
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth != uiNK64) {
            cout << FN::uiT() << "UI authorization failed from " << address.address << endl;
            document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url = "";
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            const string leaf = path.substr(path.find_last_of('.')+1);
            if (leaf == "/") {
              cout << FN::uiT() << "UI authorization success from " << address.address << endl;
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
            stringstream content;
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
          if (length > 1) {
            JSON Json;
            HandleScope hs(isolate);
            Local<Value> reply;
            string m = string(message, length).substr(2, length-2);
            MaybeLocal<Value> v = (length > 2 && (m[0] == '[' || m[0] == '{')) ? Json.Parse(isolate->GetCurrentContext(), FN::v8S(m.data())) : FN::v8S(length > 2 ? m.data() : "");
            if (sess->cb.find(string(message, 2)) != sess->cb.end()) {
              reply = (*sess->cb[string(message, 2)])(uiBIT::SNAP == (uiBIT)message[0] ? (Local<Value>)Undefined(isolate) : ((m == "true" || m == "false") ? (Local<Value>)Boolean::New(isolate, m == "true") : (v.IsEmpty() ? (Local<Value>)String::Empty(isolate) : v.ToLocalChecked())));
              if (!reply->IsUndefined() && uiBIT::SNAP == (uiBIT)message[0])
                webSocket->send(string(message, 2).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), reply->ToObject()).ToLocalChecked())).data(), uWS::OpCode::TEXT);
            }
            if (sess->_cb.find(string(message ,2)) != sess->_cb.end()) {
              Local<Value> argv[] = {uiBIT::SNAP == (uiBIT)message[0] ? (Local<Value>)Undefined(isolate) : ((m == "true" || m == "false") ? (Local<Value>)Boolean::New(isolate, m == "true") : (v.IsEmpty() ? (Local<Value>)String::Empty(isolate) : v.ToLocalChecked()))};
              reply = Local<Function>::New(isolate, sess->_cb[string(message, 2)])->Call(isolate->GetCurrentContext()->Global(), 1, argv);
              if (!reply->IsUndefined() && uiBIT::SNAP == (uiBIT)message[0])
                webSocket->send(string(message, 2).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), reply->ToObject()).ToLocalChecked())).data(), uWS::OpCode::TEXT);
            }
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if ((access("dist/sslcert/server.crt", F_OK) != -1) && (access("dist/sslcert/server.key", F_OK) != -1) && hub.listen(port, c, 0, uiGroup))
          cout << FN::uiT() << "UI ready over HTTPS on external port " << to_string(port) << "." << endl;
        else if (hub.listen(port, nullptr, 0, uiGroup))
          cout << FN::uiT() << "UI ready over HTTP on external port " << to_string(port) << "." << endl;
        else { cout << FN::uiT() << "Errrror: Use another UI port number, " << to_string(port) << " seems already in use." << endl; exit(1); }
        if (uv_timer_init(uv_default_loop(), &uiD_)) { cout << FN::uiT() << "Errrror: UV uiD_ init timer failed." << endl; exit(1); }
        uiD_.data = isolate;
        EV::evOn("QuotingParameters", [](Local<Object> qp_) {
          _uiD_(qp_->Get(FN::v8S("delayUI"))->NumberValue());
        });
        UI::uiSnap(uiTXT::ApplicationState, &onSnapApp);
        UI::uiSnap(uiTXT::Notepad, &onSnapNote);
        UI::uiHand(uiTXT::Notepad, &onHandNote);
        UI::uiSnap(uiTXT::ToggleConfigs, &onSnapOpt);
        UI::uiHand(uiTXT::ToggleConfigs, &onHandOpt);
        NODE_SET_METHOD(exports, "uiLoop", UI::uiLoop);
        NODE_SET_METHOD(exports, "uiSnap", UI::_uiSnap);
        NODE_SET_METHOD(exports, "uiHand", UI::_uiHand);
        NODE_SET_METHOD(exports, "uiSend", UI::_uiSend);
        CF::external();
      };
      static void _uiD_(double d) {
        if (uv_timer_stop(&uiD_)) { cout << FN::uiT() << "Errrror: UV uiD_ stop timer failed." << endl; exit(1); }
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        sess->D.clear();
        if (d) {
          if (uv_timer_start(&uiD_, uiD, 0, d * 1000)) { cout << FN::uiT() << "Errrror: UV uiD_ uiD start timer failed." << endl; exit(1); }
        } else {
          if (uv_timer_start(&uiD_, uiDD, 0, 60000)) { cout << FN::uiT() << "Errrror: UV uiD_ uiDD start timer failed." << endl; exit(1); }
        }
      };
      static void uiDD(uv_timer_t *handle) {
        Isolate* isolate = (Isolate*) handle->data;
        HandleScope scope(isolate);
        time_t rawtime;
        time(&rawtime);
        HeapStatistics heapStatistics;
        isolate->GetHeapStatistics(&heapStatistics);
        Local<Object> app_state = Object::New(isolate);
        app_state->Set(FN::v8S("memory"), Number::New(isolate, heapStatistics.total_heap_size()));
        app_state->Set(FN::v8S("hour"), Number::New(isolate, localtime(&rawtime)->tm_hour));
        app_state->Set(FN::v8S("freq"), Number::New(isolate, iOSR60 / 2));
        app_state->Set(FN::v8S("dbsize"), Number::New(isolate, DB::dbSize()));
        app_state->Set(FN::v8S("a"), FN::v8S(A));
        _app_state.Reset(isolate, app_state);
        iOSR60 = 0;
        uiSend(isolate, uiTXT::ApplicationState, app_state);
      };
      static void uiD(uv_timer_t *handle) {
        Isolate* isolate = (Isolate*) handle->data;
        HandleScope scope(isolate);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        for (map<uiTXT, vector<CopyablePersistentTraits<Object>::CopyablePersistent>>::iterator it_=sess->D.begin(); it_!=sess->D.end();) {
          if (it_->first != uiTXT::OrderStatusReports) {
            for (vector<CopyablePersistentTraits<Object>::CopyablePersistent>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
              uiUp(isolate, it_->first, Local<Object>::New(isolate, *it));
            it_ = sess->D.erase(it_);
          } else ++it_;
        }
        if (sess->D.find(uiTXT::OrderStatusReports) != sess->D.end() && sess->D[uiTXT::OrderStatusReports].size() > 0) {
          int ki = 0;
          Local<Array> k = Array::New(isolate);
          for (vector<CopyablePersistentTraits<Object>::CopyablePersistent>::iterator it = sess->D[uiTXT::OrderStatusReports].begin(); it != sess->D[uiTXT::OrderStatusReports].end();) {
            Local<Object> o = Local<Object>::New(isolate, *it);
            k->Set(ki++, o);
            if (mORS::Working != (mORS)o->Get(FN::v8S("orderStatus"))->NumberValue())
              it = sess->D[uiTXT::OrderStatusReports].erase(it);
            else ++it;
          }
          if (!k->GetOwnPropertyNames(Context::New(isolate)).IsEmpty())
            uiUp(isolate, uiTXT::OrderStatusReports, k);
          sess->D.erase(uiTXT::OrderStatusReports);
        }
        if (uiDDT+60000 > FN::T()) return;
        uiDDT = FN::T();
        uiDD(handle);
      };
      static Local<Value> onSnapApp(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Local<Object>::New(isolate, _app_state));
        return k;
      };
      static Local<Value> onSnapNote(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, FN::v8S(uiNOTE));
        return k;
      };
      static Local<Value> onHandNote(Local<Value> o_) {
        Isolate* isolate = Isolate::GetCurrent();
        uiNOTE = FN::S8v(o_->ToString());
        return (Local<Value>)Undefined(isolate);
      };
      static Local<Value> onSnapOpt(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Boolean::New(isolate, uiOPT));
        return k;
      };
      static Local<Value> onHandOpt(Local<Value> o_) {
        Isolate* isolate = Isolate::GetCurrent();
        uiOPT = o_->BooleanValue();
        return (Local<Value>)Undefined(isolate);
      };
      static void uiSnap(uiTXT k, uiCb cb) {
        uiOn(uiBIT::SNAP, k, cb);
      };
      static void uiHand(uiTXT k, uiCb cb) {
        uiOn(uiBIT::MSG, k, cb);
      };
      static void uiSend(Isolate* isolate, uiTXT k, Local<Object> o, bool h = false) {
        if (h) uiHold(isolate, k, o);
        else uiUp(isolate, k, o);
      };
      static void uiUp(Isolate* isolate, uiTXT k, Local<Object> o) {
        JSON Json;
        if (k == uiTXT::MarketData) {
          if (uiMDT+369 > FN::T()) return;
          uiMDT = FN::T();
        }
        MaybeLocal<String> v = o->IsUndefined() ? FN::v8S("") : Json.Stringify(isolate->GetCurrentContext(), o);
        string m = string(1, (char)uiBIT::MSG).append(string(1, (char)k)).append(*String::Utf8Value(v.ToLocalChecked()));
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      };
      static void uiOn(uiBIT k_, uiTXT _k, uiCb cb) {
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        string k = string(1, (char)k_).append(string(1, (char)_k));
        if (sess->cb.find(k) != sess->cb.end()) { cout << FN::uiT() << "Use only a single unique message handler for each \"" << k << "\" event" << endl; exit(1); }
        sess->cb[k] = cb;
      };
    private:
      static void _uiSnap(const FunctionCallbackInfo<Value>& args) {
        _uiOn(args, uiBIT::SNAP);
      };
      static void _uiHand(const FunctionCallbackInfo<Value>& args) {
        _uiOn(args, uiBIT::MSG);
      };
      static void _uiOn(const FunctionCallbackInfo<Value>& args, uiBIT k_) {
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(1, (char)k_).append(*String::Utf8Value(args[0]->ToString()));
        if (sess->_cb.find(k) != sess->_cb.end())
          return (void)isolate->ThrowException(Exception::TypeError(FN::v8S("Use only a single unique message handler for each different topic")));
        Persistent<Function> *_cb = &sess->_cb[k];
        _cb->Reset(isolate, Local<Function>::Cast(args[1]));
      };
      static void _uiSend(const FunctionCallbackInfo<Value> &args) {
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->u == 0) return;
        if (args[2]->IsUndefined() ? false : args[2]->BooleanValue()) uiHold(args.GetIsolate(), (uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->ToObject());
        else _uiUp(args);
      };
      static void _uiUp(const FunctionCallbackInfo<Value>& args) {
        if (args[1]->IsUndefined()) return;
        uiUp(args.GetIsolate(), (uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->ToObject());
      };
      static void uiHold(Isolate* isolate, uiTXT k, Local<Object> o) {
        bool isOSR = k == uiTXT::OrderStatusReports;
        if (isOSR && mORS::New == (mORS)o->Get(FN::v8S(isolate, "orderStatus"))->NumberValue()) return (void)++iOSR60;
        Local<Object> qp_ = Local<Object>::New(isolate, qpRepo);
        if (!qp_->Get(FN::v8S(isolate, "delayUI"))->NumberValue()) return uiUp(isolate, k, o);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->D.find(k) != sess->D.end() && sess->D[k].size() > 0) {
          if (!isOSR) sess->D[k].clear();
          else for (vector<CopyablePersistentTraits<Object>::CopyablePersistent>::iterator it = sess->D[k].begin(); it != sess->D[k].end();)
            if (Local<Object>::New(isolate, *it)->Get(FN::v8S(isolate, "orderId"))->ToString() == o->Get(FN::v8S(isolate, "orderId"))->ToString())
              it = sess->D[k].erase(it);
            else ++it;
        }
        Persistent<Object> _o;
        _o.Reset(isolate, o);
        sess->D[k].push_back(_o);
      };
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
      };
  };
}

#endif
