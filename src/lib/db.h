#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  sqlite3* db;
  string dbFpath;
  Persistent<Function> sqlite_;
  class DB: public node::ObjectWrap {
    public:
      static void main(Local<Object> exports) {
        Isolate* isolate = exports->GetIsolate();
        dbFpath = string("/data/db/K.").append(to_string((int)CF::cfExchange())).append(".").append(to_string((int)CF::cfBase())).append(".").append(to_string((int)CF::cfQuote())).append(".db");
        if (sqlite3_open(dbFpath.data(), &db)) { cout << sqlite3_errmsg(db) << endl; exit(1); }
        cout << "DB " << dbFpath << " loaded OK." << endl;
        Local<FunctionTemplate> o = FunctionTemplate::New(isolate, NEw);
        o->InstanceTemplate()->SetInternalFieldCount(1);
        o->SetClassName(FN::v8S("DB"));
        NODE_SET_PROTOTYPE_METHOD(o, "load", _load);
        NODE_SET_PROTOTYPE_METHOD(o, "insert", _insert);
        sqlite_.Reset(isolate, o->GetFunction());
        exports->Set(FN::v8S("DB"), o->GetFunction());
        NODE_SET_METHOD(exports, "dbSize", DB::_dbSize);
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
        JSON Json;
        MaybeLocal<String> row = Json.Stringify(isolate->GetCurrentContext(), o);
        sqlite3_exec(db,
          string((rm || id != "NULL" || time) ? string("DELETE FROM ").append(string(1, (char)k))
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append(row.IsEmpty() ? "" : string("INSERT INTO ")
            .append(string(1, (char)k)).append(" (id,json) VALUES(").append(id).append(",'")
            .append(*String::Utf8Value(row.ToLocalChecked())).append("');")).data(),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      }
    protected:
      int exchange;
      int base;
      int quote;
    private:
      explicit DB(int e_, int b_, int q_): exchange(e_), base(b_), quote(q_) {}
      static void NEw(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        if (!args.IsConstructCall())
          return (void)isolate->ThrowException(Exception::TypeError(FN::v8S("Use the 'new' operator to create new SQLite objects")));
        DB* sqlite = new DB(args[0]->NumberValue(), args[1]->NumberValue(), args[2]->NumberValue());
        sqlite->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      }
      static void _load(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(load(isolate, string(*String::Utf8Value(args[0]->ToString()))));
      }
      static int cb(void *param, int argc, char **argv, char **azColName) {
        string* json = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) json->append(argv[i]).append(",");
        return 0;
      }
      static void _insert(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        insert((uiTXT)FN::S8v(args[0]->ToString())[0], args[1]->ToObject(), args[2]->IsUndefined() ? true : args[2]->BooleanValue(), string(args[3]->IsUndefined() ? "NULL" : *String::Utf8Value(args[3]->ToString())), args[4]->IsUndefined() ? 0 : args[4]->NumberValue());
      }
      static void _dbSize(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        struct stat st;
        args.GetReturnValue().Set(Number::New(isolate, dbSize()));
      }
      static size_t dbSize() {
        struct stat st;
        return stat(dbFpath.data(), &st) != 0 ? 0 : st.st_size;
      }
  };
}

#endif
