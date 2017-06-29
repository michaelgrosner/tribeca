#ifndef K_SQLITE_H_
#define K_SQLITE_H_

#include <nan.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string>

namespace K {
  using Nan::New;
  using Nan::Set;
  using Nan::JSON;
  using Nan::GetFunction;
  using Nan::ObjectWrap;
  using Nan::Persistent;
  using Nan::MaybeLocal;
  using Nan::ThrowTypeError;
  using Nan::SetPrototypeMethod;
  using v8::Local;
  using v8::Value;
  using v8::Array;
  using v8::String;
  using v8::Object;
  using v8::Function;
  using v8::FunctionTemplate;
  using std::string;
  using std::to_string;

  class SQLite: public ObjectWrap {
    public:
      static NAN_MODULE_INIT(Init);
    protected:
      sqlite3 *db;
      int exchange;
      int base;
      int quote;
    private:
      explicit SQLite(int, int, int);
      ~SQLite();
      static NAN_METHOD(NEw);
      static NAN_METHOD(load);
      static NAN_METHOD(insert);
      static int callback(void *, int, char **, char **);
      static Persistent<Function> constructor;
  };
}

#endif
