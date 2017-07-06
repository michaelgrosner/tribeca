#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  uWS::Hub hub(0, true);
  uv_check_t check;
  Persistent<Function> noop;

  void registerCheck(Isolate *isolate) {
      uv_check_init((uv_loop_t *) hub.getLoop(), &check);
      check.data = isolate;
      uv_check_start(&check, [](uv_check_t *check) {
          Isolate *isolate = (Isolate *) check->data;
          HandleScope hs(isolate);
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, noop), 0, nullptr);
      });
      uv_unref((uv_handle_t *) &check);
  }

  class NativeString {
      char *data;
      size_t length;
      char utf8ValueMemory[sizeof(String::Utf8Value)];
      String::Utf8Value *utf8Value = nullptr;
  public:
      NativeString(const Local<Value> &value) {
          if (value->IsUndefined()) {
              data = nullptr;
              length = 0;
          } else if (value->IsString()) {
              utf8Value = new (utf8ValueMemory) String::Utf8Value(value);
              data = (**utf8Value);
              length = utf8Value->length();
          } else if (node::Buffer::HasInstance(value)) {
              data = node::Buffer::Data(value);
              length = node::Buffer::Length(value);
          } else if (value->IsTypedArray()) {
              Local<ArrayBufferView> arrayBufferView = Local<ArrayBufferView>::Cast(value);
              ArrayBuffer::Contents contents = arrayBufferView->Buffer()->GetContents();
              length = contents.ByteLength();
              data = (char *) contents.Data();
          } else if (value->IsArrayBuffer()) {
              Local<ArrayBuffer> arrayBuffer = Local<ArrayBuffer>::Cast(value);
              ArrayBuffer::Contents contents = arrayBuffer->GetContents();
              length = contents.ByteLength();
              data = (char *) contents.Data();
          } else {
              static char empty[] = "";
              data = empty;
              length = 0;
          }
      }

      char *getData() {return data;}
      size_t getLength() {return length;}
      ~NativeString() {
          if (utf8Value) {
              utf8Value->~Utf8Value();
          }
      }
  };

  struct GroupData {
      Persistent<Function> connectionHandler, messageHandler,
                           disconnectionHandler, pingHandler,
                           pongHandler, errorHandler, httpUpgradeHandler,
                           httpCancelledRequestCallback;
      int size = 0;
  };

  template <bool isServer>
  void createGroup(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<isServer> *group = hub.createGroup<isServer>(uWS::PERMESSAGE_DEFLATE, 1048576);
      group->setUserData(new GroupData);
      args.GetReturnValue().Set(External::New(args.GetIsolate(), group));
  }

  template <bool isServer>
  void deleteGroup(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      delete (GroupData *) group->getUserData();
      delete group;
  }

  template <bool isServer>
  inline Local<External> wrapSocket(uWS::WebSocket<isServer> *webSocket, Isolate *isolate) {
      return External::New(isolate, webSocket);
  }

  template <bool isServer>
  inline uWS::WebSocket<isServer> *unwrapSocket(Local<External> external) {
      return (uWS::WebSocket<isServer> *) external->Value();
  }

  inline Local<Value> wrapMessage(const char *message, size_t length, uWS::OpCode opCode, Isolate *isolate) {
      return opCode == uWS::OpCode::BINARY ? (Local<Value>) ArrayBuffer::New(isolate, (char *) message, length) : (Local<Value>) String::NewFromUtf8(isolate, message, String::kNormalString, length);
  }

  template <bool isServer>
  inline Local<Value> getDataV8(uWS::WebSocket<isServer> *webSocket, Isolate *isolate) {
      return webSocket->getUserData() ? Local<Value>::New(isolate, *(Persistent<Value> *) webSocket->getUserData()) : Local<Value>::Cast(Undefined(isolate));
  }

  template <bool isServer>
  void getUserData(const FunctionCallbackInfo<Value> &args) {
      args.GetReturnValue().Set(getDataV8(unwrapSocket<isServer>(args[0].As<External>()), args.GetIsolate()));
  }

  template <bool isServer>
  void clearUserData(const FunctionCallbackInfo<Value> &args) {
      uWS::WebSocket<isServer> *webSocket = unwrapSocket<isServer>(args[0].As<External>());
      ((Persistent<Value> *) webSocket->getUserData())->Reset();
      delete (Persistent<Value> *) webSocket->getUserData();
  }

  template <bool isServer>
  void setUserData(const FunctionCallbackInfo<Value> &args) {
      uWS::WebSocket<isServer> *webSocket = unwrapSocket<isServer>(args[0].As<External>());
      if (webSocket->getUserData()) {
          ((Persistent<Value> *) webSocket->getUserData())->Reset(args.GetIsolate(), args[1]);
      } else {
          webSocket->setUserData(new Persistent<Value>(args.GetIsolate(), args[1]));
      }
  }

  template <bool isServer>
  void getAddress(const FunctionCallbackInfo<Value> &args)
  {
      typename uWS::WebSocket<isServer>::Address address = unwrapSocket<isServer>(args[0].As<External>())->getAddress();
      Local<Array> array = Array::New(args.GetIsolate(), 3);
      array->Set(0, Integer::New(args.GetIsolate(), address.port));
      array->Set(1, String::NewFromUtf8(args.GetIsolate(), address.address));
      array->Set(2, String::NewFromUtf8(args.GetIsolate(), address.family));
      args.GetReturnValue().Set(array);
  }

  uv_handle_t *getTcpHandle(void *handleWrap) {
      volatile char *memory = (volatile char *) handleWrap;
      for (volatile uv_handle_t *tcpHandle = (volatile uv_handle_t *) memory; tcpHandle->type != UV_TCP
           || tcpHandle->data != handleWrap || tcpHandle->loop != uv_default_loop(); tcpHandle = (volatile uv_handle_t *) memory) {
          memory++;
      }
      return (uv_handle_t *) memory;
  }

  struct SendCallbackData {
      Persistent<Function> jsCallback;
      Isolate *isolate;
  };

  template <bool isServer>
  void sendCallback(uWS::WebSocket<isServer> *webSocket, void *data, bool cancelled, void *reserved)
  {
std::cout << "sendCallback" << std::endl;
      SendCallbackData *sc = (SendCallbackData *) data;
      if (!cancelled) {
          HandleScope hs(sc->isolate);
          node::MakeCallback(sc->isolate, sc->isolate->GetCurrentContext()->Global(), Local<Function>::New(sc->isolate, sc->jsCallback), 0, nullptr);
      }
      sc->jsCallback.Reset();
      delete sc;
  }

  template <bool isServer>
  void send(const FunctionCallbackInfo<Value> &args)
  {
std::cout << "sent" << std::endl;
      uWS::OpCode opCode = (uWS::OpCode) args[2]->IntegerValue();
      NativeString nativeString(args[1]);

      SendCallbackData *sc = nullptr;
      void (*callback)(uWS::WebSocket<isServer> *, void *, bool, void *) = nullptr;

      if (args[3]->IsFunction()) {
          callback = sendCallback;
          sc = new SendCallbackData;
          sc->jsCallback.Reset(args.GetIsolate(), Local<Function>::Cast(args[3]));
          sc->isolate = args.GetIsolate();
      }

      unwrapSocket<isServer>(args[0].As<External>())->send(nativeString.getData(),
                             nativeString.getLength(), opCode, callback, sc);
  }

  template <bool isServer>
  void onConnection(const FunctionCallbackInfo<Value> &args) {
std::cout << "onConnection" << (isServer ? "isServer" : "isClient") << std::endl;
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *connectionCallback = &groupData->connectionHandler;
      connectionCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      group->onConnection([isolate, connectionCallback, groupData](uWS::WebSocket<isServer> *webSocket, uWS::HttpRequest req) {
std::cout << "onConnection on" << std::endl;
          groupData->size++;
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapSocket(webSocket, isolate)};
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, *connectionCallback), 1, argv);
      });
  }

  template <bool isServer>
  void onMessage(const FunctionCallbackInfo<Value> &args) {
std::cout << "onMessage" << (isServer ? "isServer" : "isClient") << std::endl;
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *messageCallback = &groupData->messageHandler;
      messageCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      group->onMessage([isolate, messageCallback](uWS::WebSocket<isServer> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
std::cout << "onMessage on" << std::endl;
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapMessage(message, length, opCode, isolate),
                                 getDataV8(webSocket, isolate)};
          Local<Function>::New(isolate, *messageCallback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);
      });
  }

  template <bool isServer>
  void onPing(const FunctionCallbackInfo<Value> &args) {
std::cout << "onPing" << (isServer ? "isServer" : "isClient") << std::endl;
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *pingCallback = &groupData->pingHandler;
      pingCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      group->onPing([isolate, pingCallback](uWS::WebSocket<isServer> *webSocket, const char *message, size_t length) {
std::cout << "onPing on" << std::endl;
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapMessage(message, length, uWS::OpCode::PING, isolate),
                                 getDataV8(webSocket, isolate)};
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, *pingCallback), 2, argv);
      });
  }

  template <bool isServer>
  void onPong(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *pongCallback = &groupData->pongHandler;
      pongCallback->Reset(isolate, Local<Function>::Cast(args[1]));
      group->onPong([isolate, pongCallback](uWS::WebSocket<isServer> *webSocket, const char *message, size_t length) {
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapMessage(message, length, uWS::OpCode::PONG, isolate),
                                 getDataV8(webSocket, isolate)};
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, *pongCallback), 2, argv);
      });
  }

  template <bool isServer>
  void onDisconnection(const FunctionCallbackInfo<Value> &args) {
std::cout << "onDisconnection" << (isServer ? "isServer" : "isClient") << std::endl;
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *disconnectionCallback = &groupData->disconnectionHandler;
      disconnectionCallback->Reset(isolate, Local<Function>::Cast(args[1]));

      group->onDisconnection([isolate, disconnectionCallback, groupData](uWS::WebSocket<isServer> *webSocket, int code, char *message, size_t length) {
std::cout << "onDisconnection on" << std::endl;
          groupData->size--;
          HandleScope hs(isolate);
          Local<Value> argv[] = {wrapSocket(webSocket, isolate),
                                 Integer::New(isolate, code),
                                 wrapMessage(message, length, uWS::OpCode::CLOSE, isolate),
                                 getDataV8(webSocket, isolate)};
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, *disconnectionCallback), 4, argv);
      });
  }

  void onError(const FunctionCallbackInfo<Value> &args) {
std::cout << "onError" << std::endl;
      uWS::Group<uWS::CLIENT> *group = (uWS::Group<uWS::CLIENT> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *errorCallback = &groupData->errorHandler;
      errorCallback->Reset(isolate, Local<Function>::Cast(args[1]));

      group->onError([isolate, errorCallback](void *user) {
std::cout << "onError on" << std::endl;
          HandleScope hs(isolate);
          Local<Value> argv[] = {Local<Value>::New(isolate, *(Persistent<Value> *) user)};
          node::MakeCallback(isolate, isolate->GetCurrentContext()->Global(), Local<Function>::New(isolate, *errorCallback), 1, argv);

          ((Persistent<Value> *) user)->Reset();
          delete (Persistent<Value> *) user;
      });
  }

  template <bool isServer>
  void closeSocket(const FunctionCallbackInfo<Value> &args) {
      NativeString nativeString(args[2]);
      unwrapSocket<isServer>(args[0].As<External>())->close(args[1]->IntegerValue(), nativeString.getData(), nativeString.getLength());
  }

  template <bool isServer>
  void terminateSocket(const FunctionCallbackInfo<Value> &args) {
      unwrapSocket<isServer>(args[0].As<External>())->terminate();
  }

  template <bool isServer>
  void closeGroup(const FunctionCallbackInfo<Value> &args) {
      NativeString nativeString(args[2]);
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      group->close(args[1]->IntegerValue(), nativeString.getData(), nativeString.getLength());
  }

  template <bool isServer>
  void terminateGroup(const FunctionCallbackInfo<Value> &args) {
      ((uWS::Group<isServer> *) args[0].As<External>()->Value())->terminate();
  }

  template <bool isServer>
  void broadcast(const FunctionCallbackInfo<Value> &args) {
std::cout << "broadcast" << std::endl;
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      uWS::OpCode opCode = args[2]->BooleanValue() ? uWS::OpCode::BINARY : uWS::OpCode::TEXT;
      NativeString nativeString(args[1]);
      group->broadcast(nativeString.getData(), nativeString.getLength(), opCode);
  }

  template <bool isServer>
  void prepareMessage(const FunctionCallbackInfo<Value> &args) {
std::cout << "prepareMessage" << std::endl;
      uWS::OpCode opCode = (uWS::OpCode) args[1]->IntegerValue();
      NativeString nativeString(args[0]);
      args.GetReturnValue().Set(External::New(args.GetIsolate(), uWS::WebSocket<isServer>::prepareMessage(nativeString.getData(), nativeString.getLength(), opCode, false)));
  }

  template <bool isServer>
  void sendPrepared(const FunctionCallbackInfo<Value> &args) {
std::cout << "sendPrepared" << std::endl;
      unwrapSocket<isServer>(args[0].As<External>())
          ->sendPrepared((typename uWS::WebSocket<isServer>::PreparedMessage *) args[1].As<External>()->Value());
  }

  template <bool isServer>
  void finalizeMessage(const FunctionCallbackInfo<Value> &args) {
std::cout << "finalizeMessage" << std::endl;
      uWS::WebSocket<isServer>::finalizeMessage((typename uWS::WebSocket<isServer>::PreparedMessage *) args[0].As<External>()->Value());
  }

  void forEach(const FunctionCallbackInfo<Value> &args) {
      Isolate *isolate = args.GetIsolate();
      uWS::Group<uWS::SERVER> *group = (uWS::Group<uWS::SERVER> *) args[0].As<External>()->Value();
      Local<Function> cb = Local<Function>::Cast(args[1]);
      group->forEach([isolate, &cb](uWS::WebSocket<uWS::SERVER> *webSocket) {
          Local<Value> argv[] = {
              getDataV8(webSocket, isolate)
          };
          cb->Call(Null(isolate), 1, argv);
      });
  }

  void getSize(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<uWS::SERVER> *group = (uWS::Group<uWS::SERVER> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();
      args.GetReturnValue().Set(Integer::New(args.GetIsolate(), groupData->size));
  }

  void startAutoPing(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<uWS::SERVER> *group = (uWS::Group<uWS::SERVER> *) args[0].As<External>()->Value();
      NativeString nativeString(args[2]);
      group->startAutoPing(args[1]->IntegerValue(), std::string(nativeString.getData(), nativeString.getLength()));
  }

  void setNoop(const FunctionCallbackInfo<Value> &args) {
      noop.Reset(args.GetIsolate(), Local<Function>::Cast(args[0]));
  }

  void listen(const FunctionCallbackInfo<Value> &args) {
std::cout << "listen1" << to_string(args[1]->IntegerValue()) << std::endl;
      uWS::Group<uWS::SERVER> *group = (uWS::Group<uWS::SERVER> *) args[0].As<External>()->Value();
      hub.listen(args[1]->IntegerValue(), nullptr, 0, group);
  }

  template <bool isServer>
  struct Namespace {
      Local<Object> object;
      Namespace (Isolate *isolate) {
              std::cout << "Namespace" << (isServer ? "isServer" : "isClient") << std::endl;
          object = Object::New(isolate);
          NODE_SET_METHOD(object, "send", send<isServer>);
          NODE_SET_METHOD(object, "close", closeSocket<isServer>);
          NODE_SET_METHOD(object, "terminate", terminateSocket<isServer>);
          NODE_SET_METHOD(object, "prepareMessage", prepareMessage<isServer>);
          NODE_SET_METHOD(object, "sendPrepared", sendPrepared<isServer>);
          NODE_SET_METHOD(object, "finalizeMessage", finalizeMessage<isServer>);

          Local<Object> group = Object::New(isolate);
          NODE_SET_METHOD(group, "onConnection", onConnection<isServer>);
          NODE_SET_METHOD(group, "onMessage", onMessage<isServer>);
          NODE_SET_METHOD(group, "onDisconnection", onDisconnection<isServer>);

          if (!isServer) {
              NODE_SET_METHOD(group, "onError", onError);
          } else {
              NODE_SET_METHOD(group, "forEach", forEach);
              NODE_SET_METHOD(group, "getSize", getSize);
              NODE_SET_METHOD(group, "startAutoPing", startAutoPing);
              NODE_SET_METHOD(group, "listen", listen);
          }

          NODE_SET_METHOD(group, "onPing", onPing<isServer>);
          NODE_SET_METHOD(group, "onPong", onPong<isServer>);
          NODE_SET_METHOD(group, "create", createGroup<isServer>);
          NODE_SET_METHOD(group, "delete", deleteGroup<isServer>);
          NODE_SET_METHOD(group, "close", closeGroup<isServer>);
          NODE_SET_METHOD(group, "terminate", terminateGroup<isServer>);
          NODE_SET_METHOD(group, "broadcast", broadcast<isServer>);

          object->Set(String::NewFromUtf8(isolate, "group"), group);
      }
  };

  Persistent<Function> socket_;

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

      exports->Set(String::NewFromUtf8(isolate, "server"), Namespace<uWS::SERVER>(isolate).object);
      exports->Set(String::NewFromUtf8(isolate, "client"), Namespace<uWS::CLIENT>(isolate).object);

      NODE_SET_METHOD(exports, "setUserData", setUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "getUserData", getUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "clearUserData", clearUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "getAddress", getAddress<uWS::SERVER>);

      NODE_SET_METHOD(exports, "setNoop", setNoop);
      registerCheck(isolate);
    }
    protected:
      int port;
      uWS::Group<uWS::SERVER> *group;
    private:
      explicit UI(int p_): port(p_) {
std::cout << "new" << to_string(port) << std::endl;
        Isolate* isolate = Isolate::GetCurrent();

        group = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        group->setUserData(new GroupData);
        GroupData *groupData = (GroupData *) group->getUserData();

        group->onHttpRequest([&isolate](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
            std::cout << "onHttpRequest" << std::endl;
            std::cout << req.getUrl().toString() << std::endl;
            std::cout << req.getUrl().value << std::endl;
            if (req.getUrl().valueLength == 1) {
              std::stringstream content;
              if (req.getUrl().toString() == "/") {
                content << std::ifstream ("app/pub/index.html").rdbuf();
                if (!content.str().length()) isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Failed to load pub/index.html file.")));
              } else content << "Today, is a beautiful day.";
              res->end(content.str().data(), content.str().length());
            } else res->end(nullptr, 0);
        });

        std::cout << "listen: " << (hub.listen(port, nullptr, 0, group) ? "yes" : "no") << " on port " << to_string(port) << std::endl;
      }
      ~UI() {
        delete group;
      }
      static void on(const FunctionCallbackInfo<Value>& args) {
        UI* uws = ObjectWrap::Unwrap<UI>(args.This());
  std::cout << "onMessage<<<" << std::endl;
        GroupData *groupData = (GroupData *) uws->group->getUserData();

        Isolate *isolate = args.GetIsolate();
        Persistent<Function> *messageCallback = &groupData->messageHandler;
        messageCallback->Reset(isolate, Local<Function>::Cast(args[0]));
        uws->group->onMessage([isolate, messageCallback](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
  std::cout << "onMessage on" << std::endl;
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapMessage(message, length, opCode, isolate),
                                   getDataV8(webSocket, isolate)};
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
