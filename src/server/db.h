#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  class DB: public Klass {
    protected:
      void load() {
        if (sqlite3_open(argDatabase.data(), &db))
          FN::logExit("DB", sqlite3_errmsg(db), EXIT_SUCCESS);
        FN::logDB(argDatabase);
      };
    public:
      json load(uiTXT k) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string("CREATE TABLE ").append(string(1, (char)k)).append("("                                   \
          "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL," \
          "json  BLOB                                                                          NOT NULL," \
          "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);").data(),
          NULL, NULL, &zErrMsg
        );
        string j = "[";
        sqlite3_exec(db,
          string("SELECT json FROM ").append(string(1, (char)k)).append(" ORDER BY time DESC;").data(),
          cb, (void*)&j, &zErrMsg
        );
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
        if (j[strlen(j.data()) - 1] == ',') j.pop_back();
        return json::parse(j.append("]"));
      };
      void insert(uiTXT k, json o, bool rm = true, string id = "NULL", long time = 0) {
        char* zErrMsg = 0;
        sqlite3_exec(db,
          string((rm or id != "NULL" or time) ? string("DELETE FROM ").append(string(1, (char)k))
          .append(id != "NULL" ? string(" WHERE id = ").append(id).append(";") : (
            time ? string(" WHERE time < ").append(to_string(time)).append(";") : ";"
          ) ) : "").append(o.is_null() ? "" : string("INSERT INTO ")
            .append(string(1, (char)k)).append(" (id,json) VALUES(").append(id).append(",'")
            .append(o.dump()).append("');")).data(),
          NULL, NULL, &zErrMsg
        );
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
      };
      int size() {
        if (argDatabase==":memory:") return 0;
        struct stat st;
        return stat(argDatabase.data(), &st) != 0 ? 0 : st.st_size;
      };
    private:
      sqlite3* db;
      static int cb(void *param, int argc, char **argv, char **azColName) {
        string* j = reinterpret_cast<string*>(param);
        for (int i=0; i<argc; i++) j->append(argv[i]).append(",");
        return 0;
      };
  };
}

#endif
