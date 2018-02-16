#ifndef K_SH_H_
#define K_SH_H_

#ifndef K_BUILD
#define K_BUILD "0"
#endif

#ifndef K_STAMP
#define K_STAMP "0"
#endif

namespace K {
  class SH {
    private:
      future<mHotkey> hotkey;
      map<mHotkey, function<void()>*> hotFn;
      WINDOW *wBorder = nullptr,
             *wLog    = nullptr;
      int p = 0,
          spin = 0;
      string title = "?";
      multimap<mPrice, mOrder, greater<mPrice>> openOrders;
    public:
      int port = 0;
      string protocol = "?";
      mConnectivity gwConnectButton   = mConnectivity::Disconnected,
                    gwConnectExchange = mConnectivity::Disconnected;
    public:
      SH() {
        cout << BGREEN << "K" << RGREEN << " build " << K_BUILD << " " << K_STAMP << "." << BRED << '\n';
        gwEndings.push_back(&happyEnding);
      };
      void config(int argNaked, int argColors, string argExchange, string argCurrency) {
        if (argNaked) return;
        if (!(wBorder = initscr())) {
          cout << "NCURSES" << RRED << " Errrror:" << BRED << " Unable to initialize ncurses, try to run in your terminal \"export TERM=xterm\", or use --naked argument." << '\n';
          exit(EXIT_SUCCESS);
        }
        title = string(argExchange) + " " + argCurrency;
        if (argColors) start_color();
        use_default_colors();
        cbreak();
        noecho();
        keypad(wBorder, true);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        wLog = subwin(wBorder, getmaxy(wBorder)-4, getmaxx(wBorder)-2, 3, 2);
        scrollok(wLog, true);
        idlok(wLog, true);
#if CAN_RESIZE
        shResize = &resize;
        signal(SIGWINCH, sigResize);
#endif
        refresh();
        hotkeys();
        return;
      };
      static void sigResize(int sig) {
        (*shResize)();
      };
      void pressme(mHotkey ch, function<void()> *fn) {
        if (!wBorder) return;
        hotFn[ch] = fn;
      };
      void quit() {
        if (!wBorder) return;
        beep();
        endwin();
        wBorder = nullptr;
      };
      void waitForUser() {
        if (!hotkey.valid() or hotkey.wait_for(chrono::nanoseconds(0)) != future_status::ready) return;
        mHotkey ch = hotkey.get();
        if (ch == mHotkey::q or ch == mHotkey::Q)
          raise(SIGINT);
        else {
          if (hotFn.find(ch) != hotFn.end())
            (*hotFn[ch])();
          hotkeys();
        }
      };
      string stamp() {
        auto t = chrono::system_clock::now().time_since_epoch();
        auto days = chrono::duration_cast<chrono::duration<int, ratio_multiply<chrono::hours::period, ratio<24>>::type>>(t);
        t -= days;
        auto hours = chrono::duration_cast<chrono::hours>(t);
        t -= hours;
        auto minutes = chrono::duration_cast<chrono::minutes>(t);
        t -= minutes;
        auto seconds = chrono::duration_cast<chrono::seconds>(t);
        t -= seconds;
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(t);
        t -= milliseconds;
        auto microseconds = chrono::duration_cast<chrono::microseconds>(t);
        stringstream T, T_;
        T << setfill('0')
          << setw(2) << hours.count() << ":"
          << setw(2) << minutes.count() << ":"
          << setw(2) << seconds.count();
        T_ << setfill('0') << "."
           << setw(3) << milliseconds.count()
           << setw(3) << microseconds.count();
        if (!wBorder) return string(BGREEN) + T.str() + RGREEN + T_.str() + BWHITE + " ";
        wattron(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, A_BOLD);
        wprintw(wLog, T.str().data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, T_.str().data());
        wattroff(wLog, COLOR_PAIR(COLOR_GREEN));
        wprintw(wLog, " ");
        return "";
      };
      void logWar(string k, string s) {
        logErr(k, s, " Warrrrning: ");
      };
      void logErr(string k, string s, string m = " Errrror: ") {
        if (!wBorder) {
          cout << stamp() << k << RRED << m << BRED << s << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_RED));
        wprintw(wLog, m.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, s.data());
        wattroff(wLog, COLOR_PAIR(COLOR_RED));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void logDB(string k) {
        if (!wBorder) {
          cout << stamp() << "DB " << RYELLOW << k << RWHITE << " loaded OK.\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "DB ");
        wattroff(wLog, A_BOLD);
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " loaded OK.\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void logUI() {
        if (!wBorder) {
          cout << stamp() << "UI" << RWHITE << " ready over " << RYELLOW << protocol << RWHITE << " on external port " << RYELLOW << to_string(port) << RWHITE << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "UI");
        wattroff(wLog, A_BOLD);
        wprintw(wLog, " ready over ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, protocol.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " on external port ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, to_string(port).data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        refresh();
      };
      void logUIsess(int k, string s) {
        if (s.length() > 7 and s.substr(0, 7) == "::ffff:") s = s.substr(7);
        if (!wBorder) {
          cout << stamp() << "UI " << RYELLOW << to_string(k) << RWHITE << " currently connected, last connection was from " << RYELLOW << s << RWHITE << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "UI ");
        wattroff(wLog, A_BOLD);
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, to_string(k).data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, " currently connected, last connection was from ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, s.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void logVer(string k, int c) {
        if (!wBorder) {
          cout << BGREEN << "K" << RGREEN << string(" version ").append(c == -1 ? "unknown (zip install).\n" : (!c ? "0day.\n" : string("-").append(to_string(c)).append("commit").append(c > 1?"s..\n":"..\n"))) << RYELLOW << (c ? k : "") << RWHITE;
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        wattron(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, A_BOLD);
        wprintw(wLog, "K");
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" version ").append(c == -1 ? "unknown (zip install).\n" : (!c ? "0day.\n" : string("-").append(to_string(c)).append("commit").append(c > 1?"s..\n":"..\n"))).data());
        wattroff(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        if (c) wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wrefresh(wLog);
      };
      void log(mTrade k, string e) {
        if (!wBorder) {
          cout << stamp() << "GW " << (k.side == mSide::Bid ? RCYAN : RPURPLE) << e << " TRADE " << (k.side == mSide::Bid ? BCYAN : BPURPLE) << (k.side == mSide::Bid ? "BUY  " : "SELL ") << k.quantity << " " << k.pair.base << " at price " << k.price << " " << k.pair.quote << " (value " << k.value << " " << k.pair.quote << ").\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, A_BOLD);
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, "GW ");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(k.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        wprintw(wLog, string(e).append(" TRADE ").data());
        wattroff(wLog, A_BOLD);
        stringstream ss;
        ss << setprecision(8) << fixed << (k.side == mSide::Bid ? "BUY " : "SELL ") << k.quantity << " " << k.pair.base << " at price " << k.price << " " << k.pair.quote << " (value " << k.value << " " << k.pair.quote << ")";
        wprintw(wLog, ss.str().data());
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(k.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        wrefresh(wLog);
      };
      void log(string k, string s, string v) {
        if (!wBorder) {
          cout << stamp() << k << RWHITE << " " << s << " " << RYELLOW << v << RWHITE << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" ").append(s).append(" ").data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
        wprintw(wLog, v.data());
        wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void log(string k, string s) {
        if (!wBorder) {
          cout << stamp() << k << RWHITE << " " << s << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, A_BOLD);
        wprintw(wLog, string(" ").append(s).append(".\n").data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void log(string k, int c = COLOR_WHITE, bool b = false) {
        if (!wBorder) {
          cout << RWHITE << k;
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        if (b) wattron(wLog, A_BOLD);
        wattron(wLog, COLOR_PAIR(c));
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(c));
        if (b) wattroff(wLog, A_BOLD);
        wrefresh(wLog);
      };
      void log(map<mRandId, mOrder> *orders) {
        if (!wBorder) return;
        openOrders.clear();
        for (map<mRandId, mOrder>::value_type &it : *orders)
          if (mStatus::Working == it.second.orderStatus)
            openOrders.insert(pair<mPrice, mOrder>(it.second.price, it.second));
        refresh();
      };
      void refresh() {
        if (!wBorder) return;
        int l = p,
            y = getmaxy(wBorder),
            x = getmaxx(wBorder),
            k = y - openOrders.size() - 1,
            P = k;
        while (l<y) mvwhline(wBorder, l++, 1, ' ', x-1);
        if (k!=p) {
          if (k<p) wscrl(wLog, p-k);
          wresize(wLog, k-3, x-2);
          if (k>p) wscrl(wLog, p-k);
          wrefresh(wLog);
          p = k;
        }
        mvwvline(wBorder, 1, 1, ' ', y-1);
        mvwvline(wBorder, k-1, 1, ' ', y-1);
        for (map<mPrice, mOrder, greater<mPrice>>::value_type &it : openOrders) {
          wattron(wBorder, COLOR_PAIR(it.second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
          stringstream ss;
          ss << setprecision(8) << fixed << (it.second.side == mSide::Bid ? "BID" : "ASK") << " > " << it.second.exchangeId << ": " << it.second.quantity << " " << it.second.pair.base << " at price " << it.second.price << " " << it.second.pair.quote;
          mvwaddstr(wBorder, ++P, 1, ss.str().data());
          wattroff(wBorder, COLOR_PAIR(it.second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        }
        mvwaddch(wBorder, 0, 0, ACS_ULCORNER);
        mvwhline(wBorder, 0, 1, ACS_HLINE, max(80, x));
        mvwvline(wBorder, 1, 0, ACS_VLINE, k);
        mvwvline(wBorder, k, 0, ACS_LTEE, y);
        mvwaddch(wBorder, y, 0, ACS_BTEE);
        mvwaddch(wBorder, 0, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddstr(wBorder, 0, 13, (string("   ") + K_BUILD + " " + K_STAMP + " ").data());
        mvwaddch(wBorder, 0, 14, 'K' | A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, 0, 18+string(K_BUILD).length()+string(K_STAMP).length(), ACS_LTEE);
        mvwaddch(wBorder, 0, x-26, ACS_RTEE);
        mvwaddstr(wBorder, 0, x-25, (string(" [   ]: ") + (!gwConnectButton ? "Start" : "Stop?") + ", [ ]: Quit!").data());
        mvwaddch(wBorder, 0, x-9, 'q' | A_BOLD);
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, x-23, "ESC");
        wattroff(wBorder, A_BOLD);
        mvwaddch(wBorder, 0, 7, ACS_TTEE);
        mvwaddch(wBorder, 1, 7, ACS_LLCORNER);
        mvwhline(wBorder, 1, 8, ACS_HLINE, 4);
        mvwaddch(wBorder, 1, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 1, 14, title.data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, (port ? " UI on " + protocol + " port " + to_string(port) : " headless").data());
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, k, 0, ACS_LTEE);
        mvwhline(wBorder, k, 1, ACS_HLINE, 3);
        mvwaddch(wBorder, k, 4, ACS_RTEE);
        mvwaddstr(wBorder, k, 5, "< (");
        if (!gwConnectExchange) {
          wattron(wBorder, COLOR_PAIR(COLOR_RED));
          wattron(wBorder, A_BOLD);
          waddstr(wBorder, "DISCONNECTED");
          wattroff(wBorder, A_BOLD);
          wattroff(wBorder, COLOR_PAIR(COLOR_RED));
        } else if (!gwConnectButton) {
          wattron(wBorder, COLOR_PAIR(COLOR_YELLOW));
          wattron(wBorder, A_BLINK);
          waddstr(wBorder, "press START to trade");
          wattroff(wBorder, A_BLINK);
          wattroff(wBorder, COLOR_PAIR(COLOR_YELLOW));
        } else {
          wattron(wBorder, COLOR_PAIR(COLOR_YELLOW));
          waddstr(wBorder, to_string(openOrders.size()).data());
          wattroff(wBorder, COLOR_PAIR(COLOR_YELLOW));
        }
        waddstr(wBorder, ") Open Orders..");
        mvwaddch(wBorder, y-1, 0, ACS_LLCORNER);
        mvwaddstr(wBorder, 1, 2, string("|/-\\").substr(++spin, 1).data());
        if (spin==3) spin = -1;
        move(k-1, 2);
        wrefresh(wBorder);
        wrefresh(wLog);
      };
    private:
      function<void()> happyEnding = [&]() {
        quit();
        cout << '\n' << stamp() << THIS_WAS_A_TRIUMPH.str();
      };
      void hotkeys() {
        hotkey = ::async(launch::async, [&] { return (mHotkey)wgetch(wBorder); });
      };
      function<void()> resize = [&]() {
#if CAN_RESIZE
        struct winsize ws;
        if (
#ifdef _WIN32
        ioctlsocket(0, TIOCGWINSZ, &( ( unsigned long& )ws)) < 0
#else
        ioctl(0, TIOCGWINSZ, &ws) < 0
#endif
        or (ws.ws_row == getmaxy(wBorder) and ws.ws_col == getmaxx(wBorder))) return;
#endif
        werase(wBorder);
        werase(wLog);
#if CAN_RESIZE
        if (ws.ws_row < 10) ws.ws_row = 10;
        if (ws.ws_col < 30) ws.ws_col = 30;
        wresize(wBorder, ws.ws_row, ws.ws_col);
        resizeterm(ws.ws_row, ws.ws_col);
#endif
        refresh();
      };
  };
}

#endif
