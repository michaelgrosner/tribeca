#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  sqlite3* db;
  string dbFpath;
  class DB {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        dbFpath = string("/data/db/K.").append(to_string((int)CF::cfExchange())).append(".").append(to_string((int)CF::cfBase())).append(".").append(to_string((int)CF::cfQuote())).append(".db");
        if (sqlite3_open(dbFpath.data(), &db)) { cout << FN::uiT() << sqlite3_errmsg(db) << endl; exit(1); }
        cout << FN::uiT() << "DB " << dbFpath << " loaded OK." << endl;
        NODE_SET_METHOD(exports, "dbLoad", DB::_load);
        NODE_SET_METHOD(exports, "dbInsert", DB::_insert);
      }
      static Local<Value> load(Isolate* isolate, string table) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string("CREATE TABLE ").append(table).append("("                                                        \
            "id    INTEGER  PRIMARY KEY  AUTOINCREMENT        NOT NULL,"                                          \
            "json  BLOB                                       NOT NULL,"                                          \
            "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);").data(),
          NULL, NULL, &zErrMsg
        );
        string json = "[";
        sqlite3_exec(db,
          string("SELECT json FROM ").append(table).append(" ORDER BY time DESC;").data(),
          cb, (void*)&json, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        if (json[strlen(json.data()) - 1] == ',') json.pop_back();
        JSON Json;
        MaybeLocal<Value> array = Json.Parse(isolate->GetCurrentContext(), FN::v8S(json.append("]").data()));
        return array.IsEmpty() ? (Local<Value>)Array::New(isolate) : array.ToLocalChecked();
      }
      static void insert(uiTXT k, Local<Object> o, bool rm = true, string id = "NULL", long time = 0) {
        Isolate* isolate = Isolate::GetCurrent();
        char* zErrMsg = 0;
        MaybeLocal<Array> maybe_props = o->GetOwnPropertyNames(Context::New(isolate));
        MaybeLocal<String> r_;
        if (!maybe_props.IsEmpty()) {
          JSON Json;
          MaybeLocal<String> r_ = Json.Stringify(isolate->GetCurrentContext(), o);
        } else r_ = String::NewFromUtf8(isolate, "");
        string r = r_.IsEmpty() ? "" : FN::S8v(r_.ToLocalChecked());
        sqlite3_exec(db,
          string((rm || id != "NULL" || time) ? string("DELETE FROM ").append(string(1, (char)k))
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append((!r.length() || r == "{}") ? "" : string("INSERT INTO ")
            .append(string(1, (char)k)).append(" (id,json) VALUES(").append(id).append(",'")
            .append(r).append("');")).data(),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      }
      static size_t dbSize() {
        struct stat st;
        return stat(dbFpath.data(), &st) != 0 ? 0 : st.st_size;
      }
    private:
      static void _load(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(load(isolate, string(*String::Utf8Value(args[0]->ToString()))));
      }
      static void _insert(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        insert((uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->IsUndefined() ? Object::New(isolate) : args[1]->ToObject(), args[2]->IsUndefined() ? true : args[2]->BooleanValue(), string(args[3]->IsUndefined() ? "NULL" : *String::Utf8Value(args[3]->ToString())), args[4]->IsUndefined() ? 0 : args[4]->NumberValue());
      }
      static int cb(void *param, int argc, char **argv, char **azColName) {
        string* json = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) json->append(argv[i]).append(",");
        return 0;
      }
  };
}

#endif
