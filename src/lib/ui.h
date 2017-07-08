#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  uWS::Hub hub(0, true);
  uv_check_t check;
  Persistent<Function> noop_;
  Persistent<Function> socket_;
  struct GroupData {
      Persistent<Function> messageHandler;
      int size = 0;
  };
  inline Local<Value> wrapMessage(const char *message, size_t length, uWS::OpCode opCode, Isolate *isolate) {
      return opCode == uWS::OpCode::BINARY ? (Local<Value>) ArrayBuffer::New(isolate, (char *) message, length) : (Local<Value>) String::NewFromUtf8(isolate, message, String::kNormalString, length);
  }
  template <bool isServer>
  inline Local<Value> getDataV8(uWS::WebSocket<isServer> *webSocket, Isolate *isolate) {
      return webSocket->getUserData() ? Local<Value>::New(isolate, *(Persistent<Value> *) webSocket->getUserData()) : Local<Value>::Cast(Undefined(isolate));
  }
  class UI: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
      Isolate* isolate = exports->GetIsolate();
      Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
      o->InstanceTemplate()->SetInternalFieldCount(1);
      o->SetClassName(String::NewFromUtf8(isolate, "UI"));
      NODE_SET_PROTOTYPE_METHOD(o, "on", on);
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
        noop_.Reset(isolate, Local<Function>::Cast(args[0]));
        uv_check_init((uv_loop_t *) hub.getLoop(), &check);
        check.data = isolate;
        uv_check_start(&check, [](uv_check_t *check) {
          Isolate *isolate = (Isolate *) check->data;
          HandleScope hs(isolate);
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, noop_), 0, nullptr);
        });
        uv_unref((uv_handle_t *) &check);
      }
      explicit UI(int p_): port(p_) {
        Isolate* isolate = Isolate::GetCurrent();
        group = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        group->setUserData(new GroupData);
        GroupData *groupData = (GroupData *) group->getUserData();
group->onConnection([isolate, groupData](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
cout << "onConnectionnn " << endl;
    groupData->size++;
    // HandleScope hs(isolate);
    // HandleScope hs(isolate);
    // Local<Value> argv[] = {wrapSocket(webSocket, isolate)};
});
hub.onHttpUpgrade([](uWS::HttpSocket<uWS::SERVER> *s, uWS::HttpRequest req) {
cout << "onHttpUpgrade " << endl;
});
        group->onHttpRequest([isolate](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url;
            stringstream content;
            string document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            if (path.substr(1, path.find_last_of('/')-1) == "socket.io") {
              cout << "path " << path << endl;
              std::cout << to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) << endl;
              // cout << "key " <<  req.getHeader("sec-websocket-key").toString() << endl;
              // cout << "extensions " <<  req.getHeader("sec-websocket-extensions").toString() << endl;
              // cout << "protocol " <<  req.getHeader("sec-websocket-protocol").toString() << endl;
              // res->getHttpSocket()->upgrade(
                // req.getHeader("sec-websocket-key").toString().data(),
                // req.getHeader("sec-websocket-extensions").toString().data(),
                // req.getHeader("sec-websocket-extensions").toString().length(),
                // req.getHeader("sec-websocket-protocol").toString().data(),
                // req.getHeader("sec-websocket-protocol").toString().length(),
                // (bool*)new bool(true)
              // );
              content << "90:0{\"sid\":\"" << to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) << "\",\"upgrades\":[\"websocket\"],\"pingInterval\":25000,\"pingTimeout\":60000}2:40";
              document.append("Content-Type: text/plain; charset=UTF-8\r\n");
              document.append("Content-Length: ").append(to_string(content.str().length())).append("\r\n\r\n").append(content.str());
              res->write(document.data(), document.length());
            } else {
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
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if (hub.listen(port, c, 0, group))
          cout << "listen: " << "HTTPS" << " on port " << to_string(port) << endl;
        else if (hub.listen(port, nullptr, 0, group))
          cout << "listen: " << "HTTP" << " on port " << to_string(port) << endl;
      }
      ~UI() {
        delete group;
      }
      static void on(const FunctionCallbackInfo<Value>& args) {
        UI* uws = ObjectWrap::Unwrap<UI>(args.This());
        GroupData *groupData = (GroupData *) uws->group->getUserData();
        Isolate *isolate = args.GetIsolate();
        Persistent<Function> *messageCallback = &groupData->messageHandler;
        messageCallback->Reset(isolate, Local<Function>::Cast(args[0]));
        uws->group->onMessage([isolate, messageCallback](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
cout << "msgss" << endl;
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapMessage(message, length, opCode, isolate), getDataV8(webSocket, isolate)};
          Local<Function>::New(isolate, *messageCallback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);
        });
      }
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use the 'new' operator to create new UI objects")));
        UI* uws = new UI(args[0]->NumberValue());
        uws->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
  };
}

#endif
