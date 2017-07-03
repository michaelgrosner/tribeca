#ifndef K_SQLITE_H_
#define K_SQLITE_H_

namespace K {
  Persistent<Function> sqlite_;

  class SQLite: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
        o->InstanceTemplate()->SetInternalFieldCount(1);
        o->SetClassName(String::NewFromUtf8(isolate, "SQLite"));
        NODE_SET_PROTOTYPE_METHOD(o, "load", load);
        NODE_SET_PROTOTYPE_METHOD(o, "insert", insert);
        sqlite_.Reset(isolate, o->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "SQLite"), o->GetFunction());
      }
    protected:
      sqlite3 *db;
      int exchange;
      int base;
      int quote;
    private:
      explicit SQLite(int exchange_, int base_, int quote_):
        exchange(exchange_),
        base(base_),
        quote(quote_) {
        Isolate* isolate = Isolate::GetCurrent();
        if (sqlite3_open(
          string("/data/db/K.")
            .append(to_string(exchange))
            .append(".").append(to_string(base))
            .append(".").append(to_string(quote))
            .append(".db").data(),
          &db
        )) isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, sqlite3_errmsg(db))));
      }
      ~SQLite() {
        sqlite3_close(db);
      }
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Use the 'new' operator to create new SQLite objects")));

        SQLite* sqlite = new SQLite(args[0]->NumberValue(), args[1]->NumberValue(), args[2]->NumberValue());
        sqlite->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
      static void load(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);

        SQLite* sqlite = ObjectWrap::Unwrap<SQLite>(args.This());
        char* zErrMsg = 0;
        std::string table = string(*String::Utf8Value(args[0]->ToString()));
        sqlite3_exec(sqlite->db,
          string("CREATE TABLE ").append(table).append("("                                                        \
            "id    INTEGER  PRIMARY KEY  AUTOINCREMENT        NOT NULL,"                                          \
            "json  BLOB                                       NOT NULL,"                                          \
            "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);").data(),
          NULL, NULL, &zErrMsg
        );
        string json = "[";
        sqlite3_exec(sqlite->db,
          string("SELECT json FROM ").append(table).append(" ORDER BY time DESC;").data(),
          SQLite::callback, (void*)&json, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        if (json[strlen(json.data()) - 1] == ',') json.pop_back();
        JSON Json;
        MaybeLocal<Value> array = Json.Parse(isolate->GetCurrentContext(), String::NewFromUtf8(isolate, json.append("]").data()));
        if (array.IsEmpty()) args.GetReturnValue().Set(Array::New(isolate));
        else args.GetReturnValue().Set(array.ToLocalChecked());
      }
      static int callback(void *param, int argc, char **argv, char **azColName) {
        string* json = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) json->append(argv[i]).append(",");
        return 0;
      }
      static void insert(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);

        SQLite* sqlite = ObjectWrap::Unwrap<SQLite>(args.This());
        char* zErrMsg = 0;
        string table = string(*String::Utf8Value(args[0]->ToString()));
        JSON Json;
        MaybeLocal<String> row = args[1]->IsUndefined() ? String::NewFromUtf8(isolate, "") : Json.Stringify(isolate->GetCurrentContext(), args[1]->ToObject());
        bool rm = args[2]->IsUndefined() ? true : args[2]->BooleanValue();
        string id = string(args[3]->IsUndefined() ? "NULL" : *String::Utf8Value(args[3]->ToString()));
        long time = args[4]->IsUndefined() ? 0 : args[4]->NumberValue();
        sqlite3_exec(sqlite->db,
          string((rm || id != "NULL" || time) ? string("DELETE FROM ").append(table)
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append((args[1]->IsUndefined() || row.IsEmpty()) ? "" : string("INSERT INTO ")
            .append(table).append(" (id,json) VALUES(").append(id).append(",'")
            .append(*String::Utf8Value(row.ToLocalChecked())).append("');")).data(),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      }
  };
}

#endif
