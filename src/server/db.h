#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  class DB: public Klass {
    private:
      sqlite3 *db = nullptr;
    protected:
      void load() {
        if (sqlite3_open(((CF*)config)->argDatabase.data(), &db))
          exit(_errorEvent_("DB", sqlite3_errmsg(db)));
        FN::logDB(((CF*)config)->argDatabase);
        if (((CF*)config)->argDiskdata.empty()) return;
        char* zErrMsg = 0;
        sqlite3_exec(db, (
          string("ATTACH '") + ((CF*)config)->argDiskdata + "' AS qpdb;"
        ).data(), NULL, NULL, &zErrMsg);
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
        FN::logDB(((CF*)config)->argDiskdata);
      };
      void run() {
        if (((CF*)config)->argDatabase != ":memory:") return;
        size = [&]() { return 0; };
      };
    public:
      json load(mMatter k) {
        string table;
        if (k == mMatter::QuotingParameters and !((CF*)config)->argDiskdata.empty())
          table = "qpdb.";
        table += (char)k;
        char* zErrMsg = 0;
        sqlite3_exec(db, (
          string("CREATE TABLE ") + table + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);"
        ).data(), NULL, NULL, &zErrMsg);
        json j = json::array();
        sqlite3_exec(db, (
          string("SELECT json FROM ") + table + " ORDER BY time ASC;"
        ).data(), cb, (void*)&j, &zErrMsg);
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
        return j;
      };
      void insert(mMatter k, json o, bool rm = true, string id = "NULL", unsigned long expire = 0) {
        string table;
        if (k == mMatter::QuotingParameters and !((CF*)config)->argDiskdata.empty())
          table = "qpdb.";
        table += (char)k;
        ((EV*)events)->deferred([this, table, o, rm, id, expire]() {
          char* zErrMsg = 0;
          sqlite3_exec(db, (
            string((rm or id != "NULL" or expire) ? string("DELETE FROM ") + table
            + (id != "NULL" ? string(" WHERE id = ") + id  : (
              expire ? string(" WHERE time < ") + to_string(expire) : ""
            ) ) : "") + ";" + (o.is_null() ? "" : string("INSERT INTO ") + table
              + " (id,json) VALUES(" + id + ",'" + o.dump() + "');")
          ).data(), NULL, NULL, &zErrMsg);
          if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
          sqlite3_free(zErrMsg);
        });
      };
      function<unsigned int()> size = [&]() {
        struct stat st;
        return stat(((CF*)config)->argDatabase.data(), &st) ? 0 : st.st_size;
      };
    private:
      static int cb(void *param, int argc, char **argv, char **azColName) {
        for (int i = 0; i < argc; ++i)
          ((json*)param)->push_back(json::parse(argv[i]));
        return 0;
      };
  };
}

#endif
