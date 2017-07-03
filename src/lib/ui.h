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
                           pongHandler, errorHandler, httpRequestHandler,
                           httpUpgradeHandler, httpCancelledRequestCallback;
      int size = 0;
  };

  template <bool isServer>
  void createGroup(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<isServer> *group = hub.createGroup<isServer>(args[0]->IntegerValue(), args[1]->IntegerValue());
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
std::cout << "sent on" << std::endl;
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

  void connect(const FunctionCallbackInfo<Value> &args) {
std::cout << "connect on" << std::endl;
      uWS::Group<uWS::CLIENT> *clientGroup = (uWS::Group<uWS::CLIENT> *) args[0].As<External>()->Value();
      NativeString uri(args[1]);
      hub.connect(std::string(uri.getData(), uri.getLength()), new Persistent<Value>(args.GetIsolate(), args[2]), {}, 5000, clientGroup);
  }

  struct Ticket {
      uv_os_sock_t fd;
      SSL *ssl;
  };

  void upgrade(const FunctionCallbackInfo<Value> &args) {
      uWS::Group<uWS::SERVER> *serverGroup = (uWS::Group<uWS::SERVER> *) args[0].As<External>()->Value();
      Ticket *ticket = (Ticket *) args[1].As<External>()->Value();
      NativeString secKey(args[2]);
      NativeString extensions(args[3]);
      NativeString subprotocol(args[4]);

      // todo: move this check into core!
      if (ticket->fd != INVALID_SOCKET) {
          hub.upgrade(ticket->fd, secKey.getData(), ticket->ssl, extensions.getData(), extensions.getLength(), subprotocol.getData(), subprotocol.getLength(), serverGroup);
      } else {
          if (ticket->ssl) {
              SSL_free(ticket->ssl);
          }
      }
      delete ticket;
  }

  void transfer(const FunctionCallbackInfo<Value> &args) {
      // (_handle.fd OR _handle), SSL
      uv_handle_t *handle = nullptr;
      Ticket *ticket = new Ticket;
      if (args[0]->IsObject()) {
          uv_fileno((handle = getTcpHandle(args[0]->ToObject()->GetAlignedPointerFromInternalField(0))), (uv_os_fd_t *) &ticket->fd);
      } else {
          ticket->fd = args[0]->IntegerValue();
      }

      ticket->fd = dup(ticket->fd);
      ticket->ssl = nullptr;
      if (args[1]->IsExternal()) {
          ticket->ssl = (SSL *) args[1].As<External>()->Value();
          SSL_up_ref(ticket->ssl);
      }

      // uv_close calls shutdown if not set on Windows
      if (handle) {
          // UV_HANDLE_SHARED_TCP_SOCKET
          handle->flags |= 0x40000000;
      }

      args.GetReturnValue().Set(External::New(args.GetIsolate(), ticket));
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
std::cout << "onConnection on" << (isServer ? "isServer" : "isClient") << std::endl;
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
std::cout << "onMessage on" << (isServer ? "isServer" : "isClient") << std::endl;
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
std::cout << "onPing on" << (isServer ? "isServer" : "isClient") << std::endl;
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
std::cout << "onDisconnection on" << (isServer ? "isServer" : "isClient") << std::endl;
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
      uWS::Group<uWS::CLIENT> *group = (uWS::Group<uWS::CLIENT> *) args[0].As<External>()->Value();
      GroupData *groupData = (GroupData *) group->getUserData();

      Isolate *isolate = args.GetIsolate();
      Persistent<Function> *errorCallback = &groupData->errorHandler;
      errorCallback->Reset(isolate, Local<Function>::Cast(args[1]));

      group->onError([isolate, errorCallback](void *user) {
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
      uWS::Group<isServer> *group = (uWS::Group<isServer> *) args[0].As<External>()->Value();
      uWS::OpCode opCode = args[2]->BooleanValue() ? uWS::OpCode::BINARY : uWS::OpCode::TEXT;
      NativeString nativeString(args[1]);
      group->broadcast(nativeString.getData(), nativeString.getLength(), opCode);
  }

  template <bool isServer>
  void prepareMessage(const FunctionCallbackInfo<Value> &args) {
      uWS::OpCode opCode = (uWS::OpCode) args[1]->IntegerValue();
      NativeString nativeString(args[0]);
      args.GetReturnValue().Set(External::New(args.GetIsolate(), uWS::WebSocket<isServer>::prepareMessage(nativeString.getData(), nativeString.getLength(), opCode, false)));
  }

  template <bool isServer>
  void sendPrepared(const FunctionCallbackInfo<Value> &args) {
      unwrapSocket<isServer>(args[0].As<External>())
          ->sendPrepared((typename uWS::WebSocket<isServer>::PreparedMessage *) args[1].As<External>()->Value());
  }

  template <bool isServer>
  void finalizeMessage(const FunctionCallbackInfo<Value> &args) {
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
std::cout << "setNoop" << std::endl;
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
              std::cout << "createServer" << (isServer ? "isServer" : "isClient") << std::endl;
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

  Persistent<Object> reqTemplate, resTemplate;
  Persistent<Function> httpPersistent;

  uWS::HttpRequest *currentReq = nullptr;

  struct HttpServer {

      struct Request {
          static void on(const FunctionCallbackInfo<Value> &args) {
std::cout << "Request on" << std::endl;
              NativeString eventName(args[0]);
              if (std::string(eventName.getData(), eventName.getLength()) == "data") {
                  args.Holder()->SetInternalField(1, args[1]);
              } else if (std::string(eventName.getData(), eventName.getLength()) == "end") {
                  args.Holder()->SetInternalField(2, args[1]);
              } else {
                  std::cout << "Warning: req.on(" << std::string(eventName.getData(), eventName.getLength()) << ") is not implemented!" << std::endl;
              }
              args.GetReturnValue().Set(args.Holder());
          }

          static void headers(Local<String> property, const PropertyCallbackInfo<Value> &args) {
              uWS::HttpRequest *req = currentReq;
              if (!req) {
                  std::cerr << "Warning: req.headers usage past request handler is not supported!" << std::endl;
              } else {
                  NativeString nativeString(property);
                  uWS::Header header = req->getHeader(nativeString.getData(), nativeString.getLength());
                  if (header) {
                      args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) header.value, String::kNormalString, header.valueLength));
                  }
              }
          }

          static void url(Local<String> property, const PropertyCallbackInfo<Value> &args) {
              args.GetReturnValue().Set(args.This()->GetInternalField(4));
          }

          static void method(Local<String> property, const PropertyCallbackInfo<Value> &args) {
std::cout << "method" << std::endl;
              //std::cout << "method" << std::endl;
              long methodId = ((long) args.This()->GetAlignedPointerFromInternalField(3)) >> 1;
              switch (methodId) {
              case uWS::HttpMethod::METHOD_GET:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "GET", String::kNormalString, 3));
                  break;
              case uWS::HttpMethod::METHOD_PUT:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "PUT", String::kNormalString, 3));
                  break;
              case uWS::HttpMethod::METHOD_POST:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "POST", String::kNormalString, 4));
                  break;
              case uWS::HttpMethod::METHOD_HEAD:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "HEAD", String::kNormalString, 4));
                  break;
              case uWS::HttpMethod::METHOD_PATCH:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "PATCH", String::kNormalString, 5));
                  break;
              case uWS::HttpMethod::METHOD_TRACE:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "TRACE", String::kNormalString, 5));
                  break;
              case uWS::HttpMethod::METHOD_DELETE:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "DELETE", String::kNormalString, 6));
                  break;
              case uWS::HttpMethod::METHOD_OPTIONS:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "OPTIONS", String::kNormalString, 7));
                  break;
              case uWS::HttpMethod::METHOD_CONNECT:
                  args.GetReturnValue().Set(String::NewFromOneByte(args.GetIsolate(), (uint8_t *) "CONNECT", String::kNormalString, 7));
                  break;
              }
          }

          // placeholders
          static void unpipe(const FunctionCallbackInfo<Value> &args) {
              //std::cout << "req.unpipe called" << std::endl;
          }

          static void resume(const FunctionCallbackInfo<Value> &args) {
              //std::cout << "req.resume called" << std::endl;
          }

          static void socket(const FunctionCallbackInfo<Value> &args) {
              // return new empty object
              args.GetReturnValue().Set(Object::New(args.GetIsolate()));
          }

          static Local<Object> getTemplateObject(Isolate *isolate) {
              Local<FunctionTemplate> reqTemplateLocal = FunctionTemplate::New(isolate);
              reqTemplateLocal->SetClassName(String::NewFromUtf8(isolate, "uws.Request"));
              reqTemplateLocal->InstanceTemplate()->SetInternalFieldCount(5);
              reqTemplateLocal->PrototypeTemplate()->SetAccessor(String::NewFromUtf8(isolate, "url"), Request::url);
              reqTemplateLocal->PrototypeTemplate()->SetAccessor(String::NewFromUtf8(isolate, "method"), Request::method);
              reqTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "on"), FunctionTemplate::New(isolate, Request::on));
              reqTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "unpipe"), FunctionTemplate::New(isolate, Request::unpipe));
              reqTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "resume"), FunctionTemplate::New(isolate, Request::resume));
              reqTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "socket"), FunctionTemplate::New(isolate, Request::socket));

              Local<Object> reqObjectLocal = reqTemplateLocal->GetFunction()->NewInstance();

              Local<ObjectTemplate> headersTemplate = ObjectTemplate::New(isolate);
              headersTemplate->SetNamedPropertyHandler(Request::headers);

              reqObjectLocal->Set(String::NewFromUtf8(isolate, "headers"), headersTemplate->NewInstance());
              return reqObjectLocal;
          }
      };

      struct Response {
          static void on(const FunctionCallbackInfo<Value> &args) {
std::cout << "Response on" << std::endl;
              NativeString eventName(args[0]);
              if (std::string(eventName.getData(), eventName.getLength()) == "close") {
                  args.Holder()->SetInternalField(1, args[1]);
              } else {
                  std::cout << "Warning: res.on(" << std::string(eventName.getData(), eventName.getLength()) << ") is not implemented!" << std::endl;
              }
              args.GetReturnValue().Set(args.Holder());
          }

          static void end(const FunctionCallbackInfo<Value> &args) {
std::cout << "end on" << std::endl;
              uWS::HttpResponse *res = (uWS::HttpResponse *) args.Holder()->GetAlignedPointerFromInternalField(0);
              if (res) {
                  NativeString nativeString(args[0]);

                  ((Persistent<Object> *) &res->userData)->Reset();
                  ((Persistent<Object> *) &res->userData)->~Persistent<Object>();
                  ((Persistent<Object> *) &res->extraUserData)->Reset();
                  ((Persistent<Object> *) &res->extraUserData)->~Persistent<Object>();
                  res->end(nativeString.getData(), nativeString.getLength());
              }
          }

          // todo: this is slow
          static void writeHead(const FunctionCallbackInfo<Value> &args) {
              std::cout << "writeHead" << std::endl;
              
              uWS::HttpResponse *res = (uWS::HttpResponse *) args.Holder()->GetAlignedPointerFromInternalField(0);
              if (res) {
                  std::string head = "HTTP/1.1 " + std::to_string(args[0]->IntegerValue()) + " ";

                  if (args.Length() > 1 && args[1]->IsString()) {
                      NativeString statusMessage(args[1]);
                      head.append(statusMessage.getData(), statusMessage.getLength());
                  } else {
                      head += "OK";
                  }

                  if (args[args.Length() - 1]->IsObject()) {
                      Local<Object> headersObject = args[args.Length() - 1]->ToObject();
                      Local<Array> headers = headersObject->GetOwnPropertyNames();
                      for (int i = 0; i < headers->Length(); i++) {
                          Local<Value> key = headers->Get(i);
                          Local<Value> value = headersObject->Get(key);

                          NativeString nativeKey(key);
                          NativeString nativeValue(value);

                          head += "\r\n";
                          head.append(nativeKey.getData(), nativeKey.getLength());
                          head += ": ";
                          head.append(nativeValue.getData(), nativeValue.getLength());
                      }
                  }

                  head += "\r\n\r\n";
                  res->write(head.data(), head.length());
              }
          }

          // todo: if not writeHead called before then should write implicit headers
          static void write(const FunctionCallbackInfo<Value> &args) {
              uWS::HttpResponse *res = (uWS::HttpResponse *) args.Holder()->GetAlignedPointerFromInternalField(0);

              if (res) {
                  NativeString nativeString(args[0]);
                  res->write(nativeString.getData(), nativeString.getLength());
              }
          }

          static void setHeader(const FunctionCallbackInfo<Value> &args) {
              //std::cout << "res.setHeader called" << std::endl;
          }

          static void getHeader(const FunctionCallbackInfo<Value> &args) {
              //std::cout << "res.getHeader called" << std::endl;
          }

          static Local<Object> getTemplateObject(Isolate *isolate) {
              Local<FunctionTemplate> resTemplateLocal = FunctionTemplate::New(isolate);
              resTemplateLocal->SetClassName(String::NewFromUtf8(isolate, "uws.Response"));
              resTemplateLocal->InstanceTemplate()->SetInternalFieldCount(5);
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "end"), FunctionTemplate::New(isolate, Response::end));
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "writeHead"), FunctionTemplate::New(isolate, Response::writeHead));
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "write"), FunctionTemplate::New(isolate, Response::write));
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "on"), FunctionTemplate::New(isolate, Response::on));
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "setHeader"), FunctionTemplate::New(isolate, Response::setHeader));
              resTemplateLocal->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getHeader"), FunctionTemplate::New(isolate, Response::getHeader));
              return resTemplateLocal->GetFunction()->NewInstance();
          }
      };

      // todo: wrap everything up - most important function to get correct
      static void createServer(const FunctionCallbackInfo<Value> &args) {
              std::cout << "createServerHTTP" << std::endl;

          // todo: delete this on destructor
          uWS::Group<uWS::SERVER> *group = hub.createGroup<uWS::SERVER>();
          group->setUserData(new GroupData);
          GroupData *groupData = (GroupData *) group->getUserData();

          Isolate *isolate = args.GetIsolate();
          Persistent<Function> *httpRequestCallback = &groupData->httpRequestHandler;
          httpRequestCallback->Reset(isolate, Local<Function>::Cast(args[0]));
          group->onHttpRequest([isolate, httpRequestCallback](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
              std::cout << "onHttpRequest" << std::endl;
              HandleScope hs(isolate);

              currentReq = &req;

              Local<Object> reqObject = Local<Object>::New(isolate, reqTemplate)->Clone();
              reqObject->SetAlignedPointerInInternalField(0, &req);
              new (&res->extraUserData) Persistent<Object>(isolate, reqObject);

              Local<Object> resObject = Local<Object>::New(isolate, resTemplate)->Clone();
              resObject->SetAlignedPointerInInternalField(0, res);
              new (&res->userData) Persistent<Object>(isolate, resObject);

              // store url & method (needed by Koa and Express)
              long methodId = req.getMethod() << 1;
              reqObject->SetAlignedPointerInInternalField(3, (void *) methodId);
              reqObject->SetInternalField(4, String::NewFromOneByte(isolate, (uint8_t *) req.getUrl().value, String::kNormalString, req.getUrl().valueLength));

              Local<Value> argv[] = {reqObject, resObject};
              Local<Function>::New(isolate, *httpRequestCallback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);

              if (length) {
                  Local<Value> dataCallback = reqObject->GetInternalField(1);
                  if (!dataCallback->IsUndefined()) {
                      Local<Value> argv[] = {ArrayBuffer::New(isolate, data, length)};
                      Local<Function>::Cast(dataCallback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
                  }

                  if (!remainingBytes) {
                      Local<Value> endCallback = reqObject->GetInternalField(2);
                      if (!endCallback->IsUndefined()) {
                          Local<Function>::Cast(endCallback)->Call(isolate->GetCurrentContext()->Global(), 0, nullptr);
                      }
                  }
              }

              currentReq = nullptr;
              reqObject->SetAlignedPointerInInternalField(0, nullptr);
          });

          group->onCancelledHttpRequest([isolate](uWS::HttpResponse *res) {
              HandleScope hs(isolate);

              // mark res as invalid
              Local<Object> resObject = Local<Object>::New(isolate, *(Persistent<Object> *) &res->userData);
              resObject->SetAlignedPointerInInternalField(0, nullptr);

              // mark req as invalid
              Local<Object> reqObject = Local<Object>::New(isolate, *(Persistent<Object> *) &res->extraUserData);
              reqObject->SetAlignedPointerInInternalField(0, nullptr);

              // emit res 'close' on aborted response
              Local<Value> closeCallback = resObject->GetInternalField(1);
              if (!closeCallback->IsUndefined()) {
                  Local<Function>::Cast(closeCallback)->Call(isolate->GetCurrentContext()->Global(), 0, nullptr);
              }

              ((Persistent<Object> *) &res->userData)->Reset();
              ((Persistent<Object> *) &res->userData)->~Persistent<Object>();
              ((Persistent<Object> *) &res->extraUserData)->Reset();
              ((Persistent<Object> *) &res->extraUserData)->~Persistent<Object>();
          });

          group->onHttpData([isolate](uWS::HttpResponse *res, char *data, size_t length, size_t remainingBytes) {
              Local<Object> reqObject = Local<Object>::New(isolate, *(Persistent<Object> *) res->extraUserData);

              Local<Value> dataCallback = reqObject->GetInternalField(1);
              if (!dataCallback->IsUndefined()) {
                  Local<Value> argv[] = {ArrayBuffer::New(isolate, data, length)};
                  Local<Function>::Cast(dataCallback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
              }

              if (!remainingBytes) {
                  Local<Value> endCallback = reqObject->GetInternalField(2);
                  if (!endCallback->IsUndefined()) {
                      Local<Function>::Cast(endCallback)->Call(isolate->GetCurrentContext()->Global(), 0, nullptr);
                  }
              }
          });

          Local<Object> newInstance;
          if (!args.IsConstructCall()) {
              args.GetReturnValue().Set(newInstance = Local<Function>::New(args.GetIsolate(), httpPersistent)->NewInstance());
          } else {
              args.GetReturnValue().Set(newInstance = args.This());
          }

          newInstance->SetAlignedPointerInInternalField(0, group);
      }

      static void on(const FunctionCallbackInfo<Value> &args) {
          NativeString eventName(args[0]);
          std::cout << "Warning: server.on(" << std::string(eventName.getData(), eventName.getLength()) << ") is not implemented!" << std::endl;
      }

      static void listen(const FunctionCallbackInfo<Value> &args) {
          uWS::Group<uWS::SERVER> *group = (uWS::Group<uWS::SERVER> *) args.Holder()->GetAlignedPointerFromInternalField(0);
          std::cout << "listen: " << (hub.listen(args[0]->IntegerValue(), nullptr, 0, group) ? "yes" : "no") << " on port " << to_string(args[0]->IntegerValue()) << std::endl;

          if (args[args.Length() - 1]->IsFunction()) {
              Local<Function>::Cast(args[args.Length() - 1])->Call(args.GetIsolate()->GetCurrentContext()->Global(), 0, nullptr);
          }
      }

      // var app = getExpressApp(express)
      static void getExpressApp(const FunctionCallbackInfo<Value> &args) {
          Isolate *isolate = args.GetIsolate();
          if (args[0]->IsFunction()) {
              Local<Function> express = Local<Function>::Cast(args[0]);
              express->Get(String::NewFromUtf8(isolate, "request"))->ToObject()->SetPrototype(Local<Object>::New(args.GetIsolate(), reqTemplate)->GetPrototype());
              express->Get(String::NewFromUtf8(isolate, "response"))->ToObject()->SetPrototype(Local<Object>::New(args.GetIsolate(), resTemplate)->GetPrototype());

              // also change app.listen?

              // change prototypes back?

              args.GetReturnValue().Set(express->NewInstance());
          }
      }

      static void getResponsePrototype(const FunctionCallbackInfo<Value> &args) {
          args.GetReturnValue().Set(Local<Object>::New(args.GetIsolate(), resTemplate)->GetPrototype());
      }

      static void getRequestPrototype(const FunctionCallbackInfo<Value> &args) {
          args.GetReturnValue().Set(Local<Object>::New(args.GetIsolate(), reqTemplate)->GetPrototype());
      }

      static Local<Function> getHttpServer(Isolate *isolate) {
          Local<FunctionTemplate> httpServer = FunctionTemplate::New(isolate, HttpServer::createServer);
          httpServer->InstanceTemplate()->SetInternalFieldCount(1);

          httpServer->Set(String::NewFromUtf8(isolate, "createServer"), FunctionTemplate::New(isolate, HttpServer::createServer));
          httpServer->Set(String::NewFromUtf8(isolate, "getExpressApp"), FunctionTemplate::New(isolate, HttpServer::getExpressApp));
          httpServer->Set(String::NewFromUtf8(isolate, "getResponsePrototype"), FunctionTemplate::New(isolate, HttpServer::getResponsePrototype));
          httpServer->Set(String::NewFromUtf8(isolate, "getRequestPrototype"), FunctionTemplate::New(isolate, HttpServer::getRequestPrototype));
          httpServer->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "listen"), FunctionTemplate::New(isolate, HttpServer::listen));
          httpServer->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "on"), FunctionTemplate::New(isolate, HttpServer::on));

          reqTemplate.Reset(isolate, Request::getTemplateObject(isolate));
          resTemplate.Reset(isolate, Response::getTemplateObject(isolate));

          Local<Function> httpServerLocal = httpServer->GetFunction();
          httpPersistent.Reset(isolate, httpServerLocal);
          return httpServerLocal;
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
      socket_.Reset(isolate, o->GetFunction());
      exports->Set(String::NewFromUtf8(isolate, "UI"), o->GetFunction());

      exports->Set(String::NewFromUtf8(isolate, "server"), Namespace<uWS::SERVER>(isolate).object);
      exports->Set(String::NewFromUtf8(isolate, "client"), Namespace<uWS::CLIENT>(isolate).object);
      exports->Set(String::NewFromUtf8(isolate, "httpServer"), HttpServer::getHttpServer(isolate));

      NODE_SET_METHOD(exports, "setUserData", setUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "getUserData", getUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "clearUserData", clearUserData<uWS::SERVER>);
      NODE_SET_METHOD(exports, "getAddress", getAddress<uWS::SERVER>);

      NODE_SET_METHOD(exports, "transfer", transfer);
      NODE_SET_METHOD(exports, "upgrade", upgrade);
      NODE_SET_METHOD(exports, "connect", connect);
      NODE_SET_METHOD(exports, "setNoop", setNoop);
      registerCheck(isolate);
    }
    protected:
      int port;
      // thread *t;
    private:
      explicit UI(int p_): port(p_) {
        // Isolate* isolate = Isolate::GetCurrent();
std::cout << "new" << to_string(port) << std::endl;
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
