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
          exit(_redAlert_("DB", sqlite3_errmsg(db)));
        ((SH*)screen)->logDB(((CF*)config)->argDatabase);
        if (((CF*)config)->argDiskdata.empty()) return;
        qpdb = "qpdb";
        exec(string("ATTACH '") + ((CF*)config)->argDiskdata + "' AS " + qpdb + ";");
        ((SH*)screen)->logDB(((CF*)config)->argDiskdata);
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
      json load(const mMatter &table) {
        exec(string("CREATE TABLE IF NOT EXISTS ") + _table_(table) + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);");
        json result = json::array();
        exec(string("SELECT json FROM ") + _table_(table) + " ORDER BY time ASC;", &result);
        return result;
      };
      void insert(
        const mMatter &table            ,
        const json    &cell             ,
        const bool    &rm       = true  ,
        const string  &updateId = "NULL",
        const mClock  &rmOlder  = 0
      ) {
        const string sql = string(
          (rm or updateId != "NULL" or rmOlder)
            ? string("DELETE FROM ") + _table_(table) + (
              updateId != "NULL"
                ? string(" WHERE id = ") + updateId
                : (rmOlder ? string(" WHERE time < ") + to_string(rmOlder) : "")
            ) : ""
        ) + ";"
          + (cell.is_null() ? "" : string("INSERT INTO ") + _table_(table)
          + " (id,json) VALUES(" + updateId + ",'" + cell.dump() + "');");
        ((EV*)events)->deferred([this, sql]() {
          exec(sql);
        });
      };
      function<unsigned int()> size = []() { return 0; };
    private:
      inline void exec(const string &sql, json *result = nullptr) {
        char* zErrMsg = 0;
        sqlite3_exec(db, sql.data(), result ? cb : nullptr, (void*)result, &zErrMsg);
        if (zErrMsg) ((SH*)screen)->logWar("DB", string("Sqlite error: ") + zErrMsg + " at " + sql);
        sqlite3_free(zErrMsg);
      };
      static int cb(void *result, int argc, char **argv, char **azColName) {
        for (int i = 0; i < argc; ++i)
          ((json*)result)->push_back(json::parse(argv[i]));
        return 0;
      };
  };
}

#endif
