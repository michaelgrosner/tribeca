#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  static sqlite3* db;
  int sqlite3_open(string f, sqlite3** db);
  int sqlite3_exec(sqlite3* db, string q, int (*cb)(void*,int,char**,char**), void *hand, char **err);
  class DB {
    public:
      static void main() {
        if (sqlite3_open(path(), &db)) { cout << FN::uiT() << sqlite3_errmsg(db) << endl; exit(1); }
        cout << FN::uiT() << "DB " << path() << " loaded OK." << endl;
      };
      static json load(uiTXT k) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string("CREATE TABLE ").append(string(1, (char)k)).append("("                 \
          "id    INTEGER  PRIMARY KEY  AUTOINCREMENT        NOT NULL," \
          "json  BLOB                                       NOT NULL," \
          "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);"),
          NULL, NULL, &zErrMsg
        );
        string j = "[";
        sqlite3_exec(db,
          string("SELECT json FROM ").append(string(1, (char)k)).append(" ORDER BY time DESC;"),
          cb, (void*)&j, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        if (j[strlen(j.data()) - 1] == ',') j.pop_back();
        return json::parse(j.append("]"));
      };
      static void insert(uiTXT k, json o, bool rm = true, string id = "NULL", long time = 0) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string((rm or id != "NULL" or time) ? string("DELETE FROM ").append(string(1, (char)k))
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append(o.is_null() ? "" : string("INSERT INTO ")
            .append(string(1, (char)k)).append(" (id,json) VALUES(").append(id).append(",'")
            .append(o.dump()).append("');")),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) printf("sqlite error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      };
      static string path() {
        return string("/data/db/K.")
          .append(to_string((int)CF::cfExchange()))
          .append(".").append(to_string(CF::cfBase()))
          .append(".").append(to_string(CF::cfQuote()))
          .append(".db");
      };
      static int size() {
        struct stat st;
        return stat(DB::path().data(), &st) != 0 ? 0 : st.st_size;
      };
    private:
      static int cb(void *param, int argc, char **argv, char **azColName) {
        string* j = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) j->append(argv[i]).append(",");
        return 0;
      };
  };
}

#endif
