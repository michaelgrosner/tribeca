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
      void backup(mFromDb *const data, const string &ok, const string &ko = "") {
        if (data->pull(select(data)))
          screen->log("DB", explain(data, ok));
        else if (!ko.empty())
          screen->logWar("DB", explain(data, ko));
        data->push = [this, data]() {
          insert(data);
        };
      };
    private:
      json select(mFromDb *const data) {
        const string table = schema(data->about());
        json result = json::array();
        exec(
          create(table)
          + truncate(table, data->lifetime())
          + "SELECT json FROM " + table + " ORDER BY time ASC;",
          &result
        );
        return result;
      };
      void insert(mFromDb *const data) {
        const string  table    = schema(data->about());
        const json    blob     = data->dump();
        const double  limit    = data->limit();
        const mClock  lifetime = data->lifetime();
        const string  incr     = data->increment();
        const string  sql      = (
          (incr != "NULL" or !limit or lifetime)
            ? "DELETE FROM " + table + (
              incr != "NULL"
                ? " WHERE id = " + incr
                : (limit ? " WHERE time < " + to_string(Tstamp - lifetime) : "")
            ) + ";" : ""
        ) + (
          blob.is_null()
            ? ""
            : "INSERT INTO " + table
              + " (id,json) VALUES(" + incr + ",'" + blob.dump() + "');"
        );
        events->deferred([this, sql]() {
          exec(sql);
        });
      };
      string schema(const mMatter &type) {
        return (type == mMatter::QuotingParameters ? qpdb : "main") + "." + (char)type;
      };
      string create(const string &table) {
        return "CREATE TABLE IF NOT EXISTS " + table + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);";
      };
      string truncate(const string &table, const mClock &lifetime) {
        return lifetime
          ? "DELETE FROM " + table + " WHERE time < " + to_string(Tstamp - lifetime) + ";"
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
