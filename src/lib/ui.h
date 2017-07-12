#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  uWS::Hub hub(0, true);
  uv_check_t loop;
  Persistent<Function> noop;
  struct uiSession { map<string, Persistent<Function>> cb; int size = 0; };
  uWS::Group<uWS::SERVER> *uiGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
  enum uiBIT: unsigned char { MSG = '-', SNAP = '=' };
  string uiNK64 = "";
  Persistent<Function> socket_;
  class UI: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
        o->InstanceTemplate()->SetInternalFieldCount(1);
        o->SetClassName(String::NewFromUtf8(isolate, "UI"));
        NODE_SET_PROTOTYPE_METHOD(o, "on", on);
        NODE_SET_PROTOTYPE_METHOD(o, "up", up);
        socket_.Reset(isolate, o->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "UI"), o->GetFunction());
        NODE_SET_METHOD(exports, "uiLoop", UI::uiLoop);
        NODE_SET_METHOD(exports, "uiHandler", UI::uiHandler);
        // NODE_SET_METHOD(exports, "uiSnap", UI::uiSnap);
      }
    protected:
      int port;
      string name;
      string key;
    private:
      explicit UI(int p_, string n_, string k_): port(p_), name(n_), key(k_) {
        Isolate* isolate = Isolate::GetCurrent();
        uiGroup->setUserData(new uiSession);
        uiSession *session = (uiSession *) uiGroup->getUserData();
        if (name != "NULL" && key != "NULL" && name.length() > 0 && key.length() > 0) {
          B64::Encode(name.append(":").append(key), &uiNK64);
          uiNK64 = string("Basic ").append(uiNK64);
        }
        uiGroup->onConnection([session](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          session->size++;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(session->size) << " UI currently connected, last connection was from " << address.address << endl;
        });
        uiGroup->onDisconnection([session](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          session->size--;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(session->size) << " UI currently connected, last disconnection was from " << address.address << endl;
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
        uiGroup->onMessage([isolate, session](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          if (length > 1 && (session->cb.find(string(message).substr(0,2)) != session->cb.end())) {
            JSON Json;
            HandleScope hs(isolate);
            string m = string(message).substr(2, length-2);
            MaybeLocal<Value> v = (length > 2 && (m[0] == '[' || m[0] == '{')) ? Json.Parse(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, m.data())) : String::NewFromUtf8(isolate, length > 2 ? m.data() : "");
            Local<Value> argv[] = {uiBIT::SNAP == message[0] ? (Local<Value>)String::NewFromUtf8(isolate, string(message).substr(1,1).data()) : ((m == "true" || m == "false") ? (Local<Value>)Boolean::New(isolate, m == "true") : (v.IsEmpty() ? (Local<Value>)String::Empty(isolate) : v.ToLocalChecked()))};
            Local<Value> reply = Local<Function>::New(isolate, session->cb[string(message).substr(0,2)])->Call(isolate->GetCurrentContext()->Global(), 1, argv);
            if (!reply->IsUndefined() && uiBIT::SNAP == message[0])
              webSocket->send(string(message).substr(0,2).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), reply->ToObject()).ToLocalChecked())).data(), uWS::OpCode::TEXT);
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if ((access("dist/sslcert/server.crt", F_OK) != -1) && (access("dist/sslcert/server.key", F_OK) != -1) && hub.listen(port, c, 0, uiGroup))
          cout << "UI ready over " << "HTTPS" << " on external port " << to_string(port) << endl;
        else if (hub.listen(port, nullptr, 0, uiGroup))
          cout << "UI ready over " << "HTTP" << " on external port " << to_string(port) << endl;
        else isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use another UI port number, it seems already in use")));
      }
      ~UI() {
        delete uiGroup;
      }
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use the 'new' operator to create new UI objects")));
        UI* ui = new UI(args[0]->NumberValue(), string(*String::Utf8Value(args[1]->ToString())), string(*String::Utf8Value(args[2]->ToString())));
        ui->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
      static void uiSnap(const FunctionCallbackInfo<Value>& args) {
        uiOn(args, uiBIT::SNAP);
      }
      static void uiHandler(const FunctionCallbackInfo<Value>& args) {
        uiOn(args, uiBIT::MSG);
      }
      static void uiOn(const FunctionCallbackInfo<Value>& args, char k_) {
        uiSession *session = (uiSession *) uiGroup->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(1, k_).append(*String::Utf8Value(args[0]->ToString()));
        if (session->cb.find(k) != session->cb.end())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use only a single unique message handler for each different topic")));
        Persistent<Function> *cb = &session->cb[k];
        cb->Reset(isolate, Local<Function>::Cast(args[1]));
      }
      static void on(const FunctionCallbackInfo<Value>& args) {
        uiSession *session = (uiSession *) uiGroup->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(*String::Utf8Value(args[0]->ToString()));
        if (session->cb.find(k) != session->cb.end())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use only a single unique message handler for each different topic")));
        Persistent<Function> *cb = &session->cb[k];
        cb->Reset(isolate, Local<Function>::Cast(args[1]));
      }
      static void up(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        JSON Json;
        MaybeLocal<String> v = args[1]->IsUndefined() ? String::NewFromUtf8(isolate, "") : Json.Stringify(isolate->GetCurrentContext(), args[1]->ToObject());
        string m = string(1, uiBIT::MSG).append(string(*String::Utf8Value(args[0]->ToString()))).append(*String::Utf8Value(v.ToLocalChecked()));
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
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
