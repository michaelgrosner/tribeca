#ifndef K_DB_H_
#define K_DB_H_

namespace K {
  class DB: public Klass,
            public Sqlite { public: DB() { sqlite = this; };
    private:
      sqlite3 *db = nullptr;
      string qpdb = "main";
    protected:
      void load() {
        if (sqlite3_open(args.database.data(), &db))
          exit(screen->error("DB", sqlite3_errmsg(db)));
        screen->log("DB", "loaded OK from", args.database);
        if (args.diskdata.empty()) return;
        qpdb = "qpdb";
        exec("ATTACH '" + args.diskdata + "' AS " + qpdb + ";");
        screen->log("DB", "loaded OK from", args.diskdata);
      };
      void waitWebAdmin() {
        if (args.database == ":memory:") return;
        const char* path = args.database.data();
        size = [path]() {
          struct stat st;
          return stat(path, &st) ? 0 : st.st_size;
        };
      };
    public:
      void select(const mMatter &table, mFromDb *const data, const string &ok, const string &ko = "") {
        exec("CREATE TABLE IF NOT EXISTS " + schema(table) + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);");
        json result = json::array();
        exec(expire(table, data) + "SELECT json FROM " + schema(table) + " ORDER BY time ASC;", &result);
        if (!result.empty()) {
          data->read(result);
          screen->log("DB", explain(data, ok));
        } else if (!ko.empty())
          screen->logWar("DB", explain(data, ko));
      };
      void insert(
        const mMatter &table            ,
        const json    &cell             ,
        const bool    &rm       = true  ,
        const string  &updateId = "NULL",
        const mClock  &rmOlder  = 0
      ) {
        const string sql = (
          (rm or updateId != "NULL" or rmOlder)
            ? "DELETE FROM " + schema(table) + (
              updateId != "NULL"
                ? " WHERE id = " + updateId
                : (rmOlder ? " WHERE time < " + to_string(rmOlder) : "")
            ) : ""
        ) + ";"
          + (cell.is_null() ? "" : "INSERT INTO " + schema(table)
          + " (id,json) VALUES(" + updateId + ",'" + cell.dump() + "');");
        events->deferred([this, sql]() {
          exec(sql);
        });
      };
    private:
      string schema(const mMatter &table) {
        return (table == mMatter::QuotingParameters ? qpdb : "main") + "." + (char)table;
      };
      string expire(const mMatter &table, mFromDb *const data) {
        mClock rmOlder = data->expire();
        return rmOlder
          ? "DELETE FROM " + schema(table) + " WHERE time < " + to_string(_Tstamp_ - rmOlder) + ";"
          : "";
      };
      string explain(mFromDb *const data, string msg) {
        std::size_t token = msg.find("%");
        if (token != string::npos)
          msg.replace(token, 1, data->explain());
        return msg;
      };
      void exec(const string &sql, json *const result = nullptr) {
        char* zErrMsg = 0;
        sqlite3_exec(db, sql.data(), result ? write : nullptr, (void*)result, &zErrMsg);
        if (zErrMsg) screen->logWar("DB", "SQLite error: " + (zErrMsg + (" at " + sql)));
        sqlite3_free(zErrMsg);
      };
      static int write(void *result, int argc, char **argv, char **azColName) {
        for (int i = 0; i < argc; ++i)
          ((json*)result)->push_back(json::parse(argv[i]));
        return 0;
      };
  };
}

#endif
