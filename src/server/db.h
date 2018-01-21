#ifndef K_DB_H_
#define K_DB_H_

#define _table_(k) (k == mMatter::QuotingParameters ? qpdb : "main") + "." + (char)k

namespace K {
  class DB: public Klass {
    private:
      sqlite3 *db = nullptr;
      string qpdb = "main";
    protected:
      void load() {
        if (sqlite3_open(((CF*)config)->argDatabase.data(), &db))
          exit(_errorEvent_("DB", sqlite3_errmsg(db)));
        FN::logDB(((CF*)config)->argDatabase);
        if (((CF*)config)->argDiskdata.empty()) return;
        qpdb = "qpdb";
        char* zErrMsg = 0;
        sqlite3_exec(db, (
          string("ATTACH '") + ((CF*)config)->argDiskdata + "' AS " + qpdb + ";"
        ).data(), NULL, NULL, &zErrMsg);
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
        FN::logDB(((CF*)config)->argDiskdata);
      };
      void run() {
        if (((CF*)config)->argDatabase == ":memory:") return;
        const char* path = ((CF*)config)->argDatabase.data();
        size = [path]() {
          struct stat st;
          return stat(path, &st) ? 0 : st.st_size;
        };
      };
    public:
      json load(mMatter table) {
        char* zErrMsg = 0;
        sqlite3_exec(db, (
          string("CREATE TABLE ") + _table_(table) + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);"
        ).data(), NULL, NULL, &zErrMsg);
        json j = json::array();
        sqlite3_exec(db, (
          string("SELECT json FROM ") + _table_(table) + " ORDER BY time ASC;"
        ).data(), cb, (void*)&j, &zErrMsg);
        if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
        sqlite3_free(zErrMsg);
        return j;
      };
      void insert(mMatter table, json cell, bool rm = true, string updateId = "NULL", mClock rmOlder = 0) {
        ((EV*)events)->deferred([this, table, cell, rm, updateId, rmOlder]() {
          char* zErrMsg = 0;
          sqlite3_exec(db, (
            string((rm or updateId != "NULL" or rmOlder) ? string("DELETE FROM ") + _table_(table)
            + (updateId != "NULL" ? string(" WHERE id = ") + updateId  : (
              rmOlder ? string(" WHERE time < ") + to_string(rmOlder) : ""
            ) ) : "") + ";" + (cell.is_null() ? "" : string("INSERT INTO ") + _table_(table)
              + " (id,json) VALUES(" + updateId + ",'" + cell.dump() + "');")
          ).data(), NULL, NULL, &zErrMsg);
          if (zErrMsg) FN::logWar("DB", string("Sqlite error: ") + zErrMsg);
          sqlite3_free(zErrMsg);
        });
      };
      function<unsigned int()> size = []() { return 0; };
    private:
      static int cb(void *param, int argc, char **argv, char **azColName) {
        for (int i = 0; i < argc; ++i)
          ((json*)param)->push_back(json::parse(argv[i]));
        return 0;
      };
  };
}

#endif
