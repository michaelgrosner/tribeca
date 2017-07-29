#ifndef K_KV_H_
#define K_KV_H_

namespace K {
  int FN::S2mC(string k) {
    k = FN::S2u(k);
    for (unsigned i=0; i<mCurrency.size(); ++i) if (mCurrency[i] == k) return i;
    cout << FN::uiT() << "Errrror: Use of missing \"" << k << "\" currency." << endl; exit(1);
  };
  Persistent<Object> qpRepo;
  string cFname;
  json cfRepo;
  json pkRepo;
  sqlite3* db;
  string dbFpath;
  int sqlite3_open(string f, sqlite3** db);
  int sqlite3_exec(sqlite3** db, string q);
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
}

#endif
