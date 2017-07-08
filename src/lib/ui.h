#ifndef K_UI_H_
#define K_UI_H_
//gzip auth retry
namespace K {
  uWS::Hub hub(0, true);
  uv_check_t check;
  Persistent<Function> noop_;
  Persistent<Function> socket_;
  struct GroupData {
      std::map<string, Persistent<Function>> messageHandler;
      int size = 0;
  };
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
          groupData->size++;
          typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
          cout << to_string(groupData->size) << " UI connected from " << address.address << " on port " << address.port << " (" << address.family << ")" << endl;
        });
        group->onDisconnection([isolate, groupData](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          groupData->size--;
          cout << "1 UI disconnected, " << to_string(groupData->size) << " UI remain connected." << endl;
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
        group->onMessage([isolate, groupData](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          cout << "MSG" << message << endl;
          if (strcmp(message, "_") == 0) webSocket->send("¯");
          else if(length > 1 && (groupData->messageHandler.find(string(message).substr(1,1)) != groupData->messageHandler.end())) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {
              String::NewFromUtf8(isolate, string(message).substr(1,1).data()),
              String::NewFromUtf8(isolate, message, String::kNormalString, length)
            };
            Local<Function>::New(isolate, groupData->messageHandler[string(message).substr(1,1)])->Call(isolate->GetCurrentContext()->Global(), 2, argv);
          }
        });
        uS::TLS::Context c = uS::TLS::createContext("dist/sslcert/server.crt", "dist/sslcert/server.key", "");
        if (hub.listen(port, c, 0, group))
          cout << "UI ready over " << "HTTPS" << " on port " << to_string(port) << endl;
        else if (hub.listen(port, nullptr, 0, group))
          cout << "UI ready over " << "HTTP" << " on port " << to_string(port) << endl;
        else isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Unable to open port for UI, may be already in use.")));
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
        GroupData *groupData = (GroupData *) ui->group->getUserData();
        Isolate *isolate = args.GetIsolate();
        std::string k = string(*String::Utf8Value(args[0]->ToString()));
        Persistent<Function> *messageCallback = &groupData->messageHandler[k];
        messageCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      }
      static void send(const FunctionCallbackInfo<Value>& args) {
        UI* ui = ObjectWrap::Unwrap<UI>(args.This());
        Isolate *isolate = args.GetIsolate();
        std::string k = string(*String::Utf8Value(args[0]->ToString()));
        JSON Json;
        MaybeLocal<String> v = args[1]->IsUndefined() ? String::NewFromUtf8(isolate, "''") : Json.Stringify(isolate->GetCurrentContext(), args[1]->ToObject());
        std::string msg = string("['").append(k).append("',").append(*String::Utf8Value(v.ToLocalChecked())).append("]");
        ui->group->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
  };
}

#endif
