#ifndef K_KV_H_
#define K_KV_H_

namespace K {
  int FN::S2mC(string k) { k = FN::S2u(k); for (unsigned i=0; i<mCurrency.size(); ++i) if (mCurrency[i] == k) return i; cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" currency." << endl; exit(1); };
  sqlite3* db;
  int sqlite3_open(string f, sqlite3** db);
  int sqlite3_exec(sqlite3* db, string q, int (*cb)(void*,int,char**,char**), void *hand, char **err);
  uv_timer_t gwCancelAll_;
  uv_async_t gwOrder_;
  vector<mGWmt> mGWmt_;
  Persistent<Object> mGWmkt;
  json tradesMemory;
  map<string, void*> toCancel;
  map<string, json> allOrders;
  map<string, string> allOrdersIds;
}

#endif
