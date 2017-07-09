#ifndef K_UI_H_
#define K_UI_H_
//gzip auth rmcert
namespace K {
  uWS::Hub hub(0, true);
  uv_check_t loop;
  Persistent<Function> noop;
  struct Session { map<string, Persistent<Function>> cb; int size = 0; };
  Persistent<Function> socket_;
  class UI: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
        o->InstanceTemplate()->SetInternalFieldCount(1);
        o->SetClassName(String::NewFromUtf8(isolate, "UI"));
        NODE_SET_PROTOTYPE_METHOD(o, "on", on);
        NODE_SET_PROTOTYPE_METHOD(o, "send", send);
        socket_.Reset(isolate, o->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "UI"), o->GetFunction());
        NODE_SET_METHOD(exports, "setNoop", UI::setNoop);
      }
    protected:
      int port;
      uWS::Group<uWS::SERVER> *group;
    private:
      static void setNoop(const FunctionCallbackInfo<Value> &args) {
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
      explicit UI(int p_): port(p_) {
        Isolate* isolate = Isolate::GetCurrent();
        group = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        group->setUserData(new Session);
        Session *session = (Session *) group->getUserData();
        group->onConnection([isolate, session](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          session->size++;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(session->size) << " UI currently connected, connection from " << address.address << " on internal port " << address.port << " over " << address.family << endl;
        });
        group->onDisconnection([isolate, session](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          session->size--;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(session->size) << " UI currently connected, disconnection from internal port " << address.port << endl;
        });
        group->onHttpRequest([isolate](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url;
            stringstream content;
            string document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            const string leaf = path.substr(path.find_last_of('.')+1);
            if (leaf == "/") {
              document.append("Content-Type: text/html; charset=UTF-8\r\n");
              url = "/index.html";
            } else if (leaf == "js") {
              document.append("Content-Type: application/javascript; charset=UTF-8\r\n");
              url = path;
            } else if (leaf == "css") {
              document.append("Content-Type: text/css; charset=UTF-8\r\n");
              url = path;
            } else if (leaf == "png") {
              document.append("Content-Type: image/png\r\n");
              url = path;
            }
            if (!url.length()) {
              document = "HTTP/1.1 404 Not Found\r\n";
              content << "Today, is a beautiful day.";
              res->end(nullptr, 0);
            } else {
              content << ifstream (string("app/pub").append(url)).rdbuf();
              document.append("Content-Length: ").append(to_string(content.str().length())).append("\r\n\r\n").append(content.str());
              res->write(document.data(), document.length());
            }
          }
        });
        group->onMessage([isolate, session](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          if (length > 1 && (session->cb.find(string(message).substr(0,2)) != session->cb.end())) {
            JSON Json;
            HandleScope hs(isolate);
            MaybeLocal<Value> array = (length > 2 && (message[2] == '[' || message[2] == '{')) ? Json.Parse(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, string(message).substr(2, length-2).data())) : String::NewFromUtf8(isolate, length > 2 ? string(message).substr(2, length-2).data() : "");
            Local<Value> argv[] = {String::NewFromUtf8(isolate, ""), array.IsEmpty() ? (Local<Value>)Array::New(isolate) : array.ToLocalChecked()};
            Local<Value> reply = Local<Function>::New(isolate, session->cb[string(message).substr(0,2)])->Call(isolate->GetCurrentContext()->Global(), 2, argv);
            if (!reply->IsUndefined()) webSocket->send(string("=").append(string(message).substr(1,1)).append(*String::Utf8Value(Json.Stringify(isolate->GetCurrentContext(), reply->ToObject()).ToLocalChecked())).data(), uWS::OpCode::TEXT);
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if (hub.listen(port, c, 0, group))
          cout << "UI ready over " << "HTTPS" << " on external port " << to_string(port) << endl;
        else if (hub.listen(port, nullptr, 0, group))
          cout << "UI ready over " << "HTTP" << " on external port " << to_string(port) << endl;
        else isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use another UI port number, it seems already in use")));
      }
      ~UI() {
        delete group;
      }
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use the 'new' operator to create new UI objects")));
        UI* ui = new UI(args[0]->NumberValue());
        ui->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
      static void on(const FunctionCallbackInfo<Value>& args) {
        UI* ui = ObjectWrap::Unwrap<UI>(args.This());
        Session *session = (Session *) ui->group->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(*String::Utf8Value(args[0]->ToString()));
        if (session->cb.find(k) != session->cb.end())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use a single message handler for each different topic")));
        Persistent<Function> *messageCallback = &session->cb[k];
        messageCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      }
      static void send(const FunctionCallbackInfo<Value>& args) {
        UI* ui = ObjectWrap::Unwrap<UI>(args.This());
        Session *session = (Session *) ui->group->getUserData();
        Isolate *isolate = args.GetIsolate();
        string k = string(*String::Utf8Value(args[0]->ToString()));
        JSON Json;
        MaybeLocal<String> v = args[1]->IsUndefined() ? String::NewFromUtf8(isolate, "") : Json.Stringify(isolate->GetCurrentContext(), args[1]->ToObject());
        string msg = k.append(*String::Utf8Value(v.ToLocalChecked()));
        ui->group->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
  };
}

#endif
