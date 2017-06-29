#include "sqlite.h"

namespace K {
  NAN_MODULE_INIT(SQLite::Init) {
    Local<FunctionTemplate> o = New<FunctionTemplate>(NEw);
    o->InstanceTemplate()->SetInternalFieldCount(1);
    o->SetClassName(New("SQLite").ToLocalChecked());
    SetPrototypeMethod(o, "load", load);
    SetPrototypeMethod(o, "insert", insert);
    constructor.Reset(GetFunction(o).ToLocalChecked());
    Set(target, New("SQLite").ToLocalChecked(), GetFunction(o).ToLocalChecked());
  }

  Persistent<Function> SQLite::constructor;

  SQLite::SQLite(int exchange_, int base_, int quote_):
  exchange(exchange_),
  base(base_),
  quote(quote_) {
    if (sqlite3_open(
      string("/data/db/K.")
        .append(to_string(exchange))
        .append(".").append(to_string(base))
        .append(".").append(to_string(quote))
        .append(".db").data(),
      &db
    )) ThrowTypeError(sqlite3_errmsg(db));
  }

  SQLite::~SQLite() {
    sqlite3_close(db);
  }

  NAN_METHOD(SQLite::NEw) {
    if (!info.IsConstructCall())
      return ThrowTypeError("Use the 'new' operator to create new SQLite objects");
    SQLite* sqlite = new SQLite(
      info[0]->NumberValue(),
      info[1]->NumberValue(),
      info[2]->NumberValue()
    );
    sqlite->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  NAN_METHOD(SQLite::load) {
    SQLite* sqlite = ObjectWrap::Unwrap<SQLite>(info.This());
    char* zErrMsg = 0;
    string table = string(*Nan::Utf8String(Local<String>::Cast(info[0])));
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
    MaybeLocal<Value> array = Json.Parse(New(json.append("]").data()).ToLocalChecked());
    if (array.IsEmpty()) info.GetReturnValue().Set(New<Array>());
    else info.GetReturnValue().Set(array.ToLocalChecked());
  }

  int SQLite::callback(void *param, int argc, char **argv, char **azColName) {
    string* json = reinterpret_cast<string*>(param);
    for(int i=0; i<argc; i++) json->append(argv[i]).append(",");
    return 0;
  }

  NAN_METHOD(SQLite::insert) {
    SQLite* sqlite = ObjectWrap::Unwrap<SQLite>(info.This());
    char* zErrMsg = 0;
    string table = string(*Nan::Utf8String(Local<String>::Cast(info[0])));
    JSON Json;
    MaybeLocal<String> row = Json.Stringify(info[1]->IsUndefined() ? New<Object>() : Nan::To<Object>(info[1]).ToLocalChecked());
    bool rm = info[2]->IsUndefined() ? true : info[2]->BooleanValue();
    string id = string(info[3]->IsUndefined() ? "NULL" : *Nan::Utf8String(Local<String>::Cast(info[3])));
    long time = info[4]->IsUndefined() ? 0 : info[4]->NumberValue();
    sqlite3_exec(sqlite->db,
      string((rm || id != "NULL" || time) ? string("DELETE FROM ").append(table)
      .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
        time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
      ) ) : "").append(info[1]->IsUndefined() ? "" : string("INSERT INTO ")
        .append(table).append(" (id,json) VALUES(").append(id).append(",'")
        .append(*Nan::Utf8String(row.ToLocalChecked())).append("');")).data(),
      NULL, NULL, &zErrMsg
    );
    if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
}
