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
        group->onHttpRequest([&isolate](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          stringstream content;
          string document = "HTTP/1.1 200 OK\r\nnConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\n";
          string path = req.getUrl().toString();
          string::size_type n = 0;
          while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
          const string leaf = path.substr(path.find_last_of('.')+1);
          if (leaf == "/") {
            document.append("Content-Type: text/html; charset=UTF-8\r\n");
            content << ifstream ("app/pub/index.html").rdbuf();
            if (!content.str().length()) isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Failed to load pub/index.html file.")));
          } else if (leaf == "js" || leaf == "css" || leaf == "png") {
            if (leaf == "js") document.append("Content-Type: application/javascript; charset=UTF-8\r\n");
            if (leaf == "css") document.append("Content-Type: text/css; charset=UTF-8\r\n");
            if (leaf == "png") document.append("Content-Type: image/png\r\n");
            content << ifstream (string("app/pub").append(path)).rdbuf();
            if (!content.str().length()) isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Failed to load pub/index.html file.")));
          } else content << "Today, is a beautiful day.";
          document.append("Content-Length: ").append(to_string(content.str().length())).append("\r\n\r\n").append(content.str());
          res->write(document.data(), document.length());
        });
        cout << "listen: " << (hub.listen(port, nullptr, 0, group) ? "yes" : "no") << " on port " << to_string(port) << endl;
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
