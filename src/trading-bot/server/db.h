#ifndef K_DB_H_
#define K_DB_H_

class DB: public Sqlite { public: DB() { sqlite = this; };
  private:
    sqlite3 *db = nullptr;
    string qpdb = "main";
  protected:
    void load() override {
      if (sqlite3_open(K.option.str("database").data(), &db))
        error("DB", sqlite3_errmsg(db));
      Print::log("DB", "loaded OK from", K.option.str("database"));
      if (K.option.str("diskdata").empty()) return;
      qpdb = "qpdb";
      exec("ATTACH '" + K.option.str("diskdata") + "' AS " + qpdb + ";");
      Print::log("DB", "loaded OK from", K.option.str("diskdata"));
    };
  public:
    void backup(mFromDb *const data) override {
      const bool loaded = data->pull(select(data));
      const string msg = data->explanation(loaded);
      data->push = [this, data]() { insert(data); };
      if (msg.empty()) return;
      if (loaded) Print::log("DB", msg);
      else Print::logWar("DB", msg);
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
      const json    blob     = data->blob();
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
    void exec(const string &sql, json *const result = nullptr) {                // Print::log("DB DEBUG", sql);
      char* zErrMsg = nullptr;
      sqlite3_exec(db, sql.data(), result ? write : nullptr, (void*)result, &zErrMsg);
      if (zErrMsg) Print::logWar("DB", "SQLite error: " + (zErrMsg + (" at " + sql)));
      sqlite3_free(zErrMsg);
    };
    static int write(void *result, int argc, char **argv, char **azColName) {
      for (int i = 0; i < argc; ++i)
        ((json*)result)->push_back(json::parse(argv[i]));
      return 0;
    };
} db;

#endif
