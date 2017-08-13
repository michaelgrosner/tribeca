#ifndef K_KV_H_
#define K_KV_H_

namespace K {
  string A;
  int FN::S2mC(string k) { k = FN::S2u(k); for (unsigned i=0; i<mCurrency.size(); ++i) if (mCurrency[i] == k) return i; cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" currency." << endl; exit(1); };
  Persistent<Object> qpRepo;
  string cFname;
  json cfRepo;
  json pkRepo;
  sqlite3* db;
  string dbFpath;
  int sqlite3_open(string f, sqlite3** db);
  int sqlite3_exec(sqlite3* db, string q, int (*cb)(void*,int,char**,char**), void *hand, char **err);
  uWS::Hub hub(0, true);
  uv_check_t loop;
  uv_timer_t uiD_;
  Persistent<Function> noop;
  typedef Local<Value> (*uiCb)(Local<Value>);
  struct uiSess { map<string, Persistent<Function>> _cb; map<string, uiCb> cb; map<uiTXT, vector<CopyablePersistentTraits<Object>::CopyablePersistent>> D; int u = 0; };
  uWS::Group<uWS::SERVER> *uiGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
  int iOSR60 = 0;
  bool uiOPT = true;
  unsigned long uiMDT = 0;
  unsigned long uiDDT = 0;
  string uiNOTE = "";
  string uiNK64 = "";
  Persistent<Function> socket_;
  Persistent<Object> _app_state;
  uv_timer_t gwRec_;
  uv_timer_t gwPos_;
  uv_timer_t gwBook_;
  uv_timer_t gwTrade_;
  uv_timer_t gwCancelAll_;
  uv_async_t gwFix_;
  pthread_mutex_t gwMutex_;
  bool gwAutoStart = false;
  bool gwState = false;
  mConnectivityStatus gwConn = mConnectivityStatus::Disconnected;
  mConnectivityStatus gwMDConn = mConnectivityStatus::Disconnected;
  mConnectivityStatus gwEOConn = mConnectivityStatus::Disconnected;
  Gw* gw;
  Gw* gw_;
  vector<mGWmt> mGWmt_;
  Persistent<Object> mGWmkt;
}

#endif
