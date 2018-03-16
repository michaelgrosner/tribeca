#ifndef K_SH_H_
#define K_SH_H_

#define _redAlert_ ((SH*)screen)->error

namespace K {
  vector<function<void()>*> endingFn;
  char RBLACK[] = "\033[0;30m", RRED[]    = "\033[0;31m", RGREEN[] = "\033[0;32m", RYELLOW[] = "\033[0;33m",
       RBLUE[]  = "\033[0;34m", RPURPLE[] = "\033[0;35m", RCYAN[]  = "\033[0;36m", RWHITE[]  = "\033[0;37m",
       BBLACK[] = "\033[1;30m", BRED[]    = "\033[1;31m", BGREEN[] = "\033[1;32m", BYELLOW[] = "\033[1;33m",
       BBLUE[]  = "\033[1;34m", BPURPLE[] = "\033[1;35m", BCYAN[]  = "\033[1;36m", BWHITE[]  = "\033[1;37m",
       RRESET[] = "\033[0m";
  class SH {
    private:
      future<mHotkey> hotkey;
      map<mHotkey, function<void()>*> hotFn;
      WINDOW *wBorder = nullptr,
             *wLog    = nullptr;
      int cursor = 0,
          spinOrders = 0,
          port = 0,
          baseAmount = 0,
          baseHeld = 0,
          quoteAmount = 0,
          quoteHeld = 0;
      string title = "?",
             base = "?",
             quote = "?",
             baseValue = "?",
             quoteValue = "?",
             fairValue = "?";
      multimap<mPrice, mOrder, greater<mPrice>> openOrders;
    public:
      string protocol = "HTTP";
      mConnectivity *gwConnected         = nullptr,
                    *gwConnectedExchange = nullptr;

    public:
      SH() {
        cout << BGREEN << "K" << RGREEN << " build " << K_BUILD << ' ' << K_STAMP << '.' << BRED << '\n';
        endingFn.push_back(&happyEnding);
        if (access(".git", F_OK) != -1) {
          system("git fetch");
          string k = changelog();
          logVer(k, count(k.begin(), k.end(), '\n'));
        } else logVer("", -1);
        cout << RRESET;
      };
      void config(string base_, string quote_, string argExchange, int argColors, int argPort) {
        if (!(wBorder = initscr())) {
          cout << "NCURSES" << RRED << " Errrror:" << BRED << " Unable to initialize ncurses, try to run in your terminal \"export TERM=xterm\", or use --naked argument." << '\n';
          exit(EXIT_SUCCESS);
        }
        base = base_;
        quote = quote_;
        title = argExchange;
        port = argPort;
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
        wLog = subwin(wBorder, getmaxy(wBorder)-4, getmaxx(wBorder)-2-6, 3, 2);
        scrollok(wLog, true);
        idlok(wLog, true);
#if CAN_RESIZE
        signal(SIGWINCH, resize);
#endif
        refresh();
        hotkeys();
      };
      static string changelog() {
        return FN::output("test -d .git && git --no-pager log --graph --oneline @..@{u}");
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
      int error(string k, string s, bool reboot = false) {
        quit();
        logErr(k, s);
        cout << RRESET;
        return reboot ? EXIT_FAILURE : EXIT_SUCCESS;
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
          << setw(2) << hours.count() << ':'
          << setw(2) << minutes.count() << ':'
          << setw(2) << seconds.count();
        T_ << setfill('0') << '.'
           << setw(3) << milliseconds.count()
           << setw(3) << microseconds.count();
        if (!wBorder) return string(BGREEN) + T.str() + RGREEN + T_.str() + BWHITE + ' ';
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
        cout << BGREEN << K_0_DAY << RGREEN << string(c == -1 ? " (zip install).\n" : (!c ? " (0day).\n" : string(" -").append(to_string(c)).append("commit").append(c > 1?"s..\n":"..\n"))) << RYELLOW << (c ? k : "") << RWHITE;
      };
      void log(mTrade k, string e) {
        if (!wBorder) {
          cout << stamp() << "GW " << (k.side == mSide::Bid ? RCYAN : RPURPLE) << e << " TRADE " << (k.side == mSide::Bid ? BCYAN : BPURPLE) << (k.side == mSide::Bid ? "BUY  " : "SELL ") << k.quantity << ' ' << k.pair.base << " at price " << k.price << ' ' << k.pair.quote << " (value " << k.value << ' ' << k.pair.quote << ").\n";
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
        wprintw(wLog, (string(k.side == mSide::Bid ? "BUY  " : "SELL ")
          + FN::str8(k.quantity) + ' ' + k.pair.base + " at price "
          + FN::str8(k.price) + ' ' + k.pair.quote + " (value "
          + FN::str8(k.value) + ' ' + k.pair.quote + ")"
        ).data());
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(k.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        wrefresh(wLog);
      };
      void log(string k, string s, string v) {
        if (!wBorder) {
          cout << stamp() << k << RWHITE << ' ' << s << ' ' << RYELLOW << v << RWHITE << ".\n";
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
          cout << stamp() << k << RWHITE << ' ' << s << ".\n";
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
      void log(map<mRandId, mOrder> *orders, bool working) {
        if (!wBorder) return;
        openOrders.clear();
        for (map<mRandId, mOrder>::value_type &it : *orders)
          if (mStatus::Working == it.second.orderStatus)
            openOrders.insert(pair<mPrice, mOrder>(it.second.price, it.second));
        if (working and ++spinOrders == 4) spinOrders = 0;
        refresh();
      };
      void log(mPosition &pos) {
        if (!wBorder) return;
        baseAmount = round(pos.baseAmount * 10 / pos.baseValue);
        baseHeld = round(pos.baseHeldAmount * 10 / pos.baseValue);
        quoteAmount = round(pos.quoteAmount * 10 / pos.quoteValue);
        quoteHeld = round(pos.quoteHeldAmount * 10 / pos.quoteValue);
        _fixed8_(pos.baseValue, baseValue)
        _fixed8_(pos.quoteValue, quoteValue)
        refresh();
      };
      void log(double fv) {
        if (!wBorder) return;
        _fixed8_(fv, fairValue)
        refresh();
      };
      void refresh() {
        if (!wBorder) return;
        int lastcursor = cursor,
            y = getmaxy(wBorder),
            x = getmaxx(wBorder),
            yMaxLog = y - max((int)openOrders.size(), (!gwConnected or !*gwConnected) ? 0 : 2) - 1,
            yOrders = yMaxLog;
        while (lastcursor<y) mvwhline(wBorder, lastcursor++, 1, ' ', x-1);
        if (yMaxLog!=cursor) {
          if (yMaxLog<cursor) wscrl(wLog, cursor-yMaxLog);
          wresize(wLog, yMaxLog-3, x-2-6);
          if (yMaxLog>cursor) wscrl(wLog, cursor-yMaxLog);
          wrefresh(wLog);
          cursor = yMaxLog;
        }
        mvwvline(wBorder, 1, 1, ' ', y-1);
        mvwvline(wBorder, yMaxLog-1, 1, ' ', y-1);
        for (map<mPrice, mOrder, greater<mPrice>>::value_type &it : openOrders) {
          wattron(wBorder, COLOR_PAIR(it.second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
          mvwaddstr(wBorder, ++yOrders, 1, (string(it.second.side == mSide::Bid ? "BID" : "ASK") + " > "
            + FN::str8(it.second.quantity) + ' ' + it.second.pair.base + " at price "
            + FN::str8(it.second.price) + ' ' + it.second.pair.quote + " (value "
            + FN::str8(abs(it.second.price * it.second.quantity)) + ' ' + it.second.pair.quote + ")"
          ).data());
          wattroff(wBorder, COLOR_PAIR(it.second.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        }
        mvwaddch(wBorder, 0, 0, ACS_ULCORNER);
        mvwhline(wBorder, 0, 1, ACS_HLINE, max(80, x));
        mvwhline(wBorder, 1, 14, ' ', max(80, x)-14);
        mvwvline(wBorder, 1, 0, ACS_VLINE, yMaxLog);
        mvwvline(wBorder, yMaxLog, 0, ACS_LTEE, y);
        mvwaddch(wBorder, y, 0, ACS_BTEE);
        mvwaddch(wBorder, 0, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        string title1 = string("   ") + title;
        string title2 = string(" ") + (port ? "UI on " + protocol + " port " + to_string(port) : "headless")  + ' ';
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, 13, title1.data());
        wattroff(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, 13+title1.length(), title2.data());
        mvwaddch(wBorder, 0, 14, 'K' | A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, 0, 13+title1.length()+title2.length(), ACS_LTEE);
        mvwaddch(wBorder, 0, x-26, ACS_RTEE);
        mvwaddstr(wBorder, 0, x-25, (string(" [   ]: ") + ((!gwConnected or !*gwConnected) ? "Start" : "Stop?") + ", [ ]: Quit!").data());
        mvwaddch(wBorder, 0, x-9, 'q' | A_BOLD);
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, x-23, "ESC");
        wattroff(wBorder, A_BOLD);
        mvwaddch(wBorder, 0, 7, ACS_TTEE);
        mvwaddch(wBorder, 1, 7, ACS_LLCORNER);
        mvwhline(wBorder, 1, 8, ACS_HLINE, 4);
        mvwaddch(wBorder, 1, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        wattron(wBorder, A_BOLD);
        waddstr(wBorder, (string(" ") + baseValue + ' ').data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, base.data());
        wattroff(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        waddstr(wBorder, " or ");
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        wattron(wBorder, COLOR_PAIR(COLOR_CYAN));
        wattron(wBorder, A_BOLD);
        waddstr(wBorder, quoteValue.data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, (string(" ") + quote + ' ').data());
        wattroff(wBorder, COLOR_PAIR(COLOR_CYAN));
        size_t xLenValue = 14+baseValue.length()+quoteValue.length()+base.length()+quote.length()+7,
               xMaxValue = max(xLenValue+1, 18+title1.length()+title2.length());
        mvwaddch(wBorder, 0, xMaxValue, ACS_TTEE);
        mvwaddch(wBorder, 1, xMaxValue, ACS_LRCORNER);
        mvwhline(wBorder, 1, xLenValue, ACS_HLINE, xMaxValue - xLenValue);
        mvwaddch(wBorder, 1, xLenValue, ACS_LTEE);
        int yPos = max(1, (y / 2) - 6);
        mvwvline(wBorder, yPos+1, x-3, ' ', 10);
        mvwvline(wBorder, yPos+1, x-4, ' ', 10);
        wattron(wBorder, COLOR_PAIR(COLOR_CYAN));
        mvwvline(wBorder, yPos+11-quoteAmount-quoteHeld, x-4, ACS_CKBOARD, quoteHeld);
        wattron(wBorder, A_BOLD);
        mvwvline(wBorder, yPos+11-quoteAmount, x-4, ' ' | A_REVERSE, quoteAmount);
        wattroff(wBorder, A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_CYAN));
        wattron(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        mvwvline(wBorder, yPos+11-baseAmount-baseHeld, x-3, ACS_CKBOARD, baseHeld);
        wattron(wBorder, A_BOLD);
        mvwvline(wBorder, yPos+11-baseAmount, x-3, ' ' | A_REVERSE, baseAmount);
        wattroff(wBorder, A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        mvwaddch(wBorder, yPos, x-2, ACS_URCORNER);
        mvwaddch(wBorder, yPos+11, x-2, ACS_LRCORNER);
        mvwaddch(wBorder, yPos, x-5, ACS_ULCORNER);
        mvwaddch(wBorder, yPos+11, x-5, ACS_LLCORNER);
        mvwhline(wBorder, yPos, x-4, ACS_HLINE, 2);
        mvwhline(wBorder, yPos+11, x-4, ACS_HLINE, 2);
        mvwaddch(wBorder, yMaxLog, 0, ACS_LTEE);
        mvwhline(wBorder, yMaxLog, 1, ACS_HLINE, 3);
        mvwaddch(wBorder, yMaxLog, 4, ACS_RTEE);
        mvwaddstr(wBorder, yMaxLog, 5, "< (");
        if (!gwConnectedExchange or !*gwConnectedExchange) {
          wattron(wBorder, COLOR_PAIR(COLOR_RED));
          wattron(wBorder, A_BOLD);
          waddstr(wBorder, "DISCONNECTED");
          wattroff(wBorder, A_BOLD);
          wattroff(wBorder, COLOR_PAIR(COLOR_RED));
          waddch(wBorder, ')');
        } else {
          if (!gwConnected or !*gwConnected) {
            wattron(wBorder, COLOR_PAIR(COLOR_YELLOW));
            wattron(wBorder, A_BLINK);
            waddstr(wBorder, "press START to trade");
            wattroff(wBorder, A_BLINK);
            wattroff(wBorder, COLOR_PAIR(COLOR_YELLOW));
            waddch(wBorder, ')');
          } else {
            wattron(wBorder, COLOR_PAIR(COLOR_YELLOW));
            waddstr(wBorder, to_string(openOrders.size()).data());
            wattroff(wBorder, COLOR_PAIR(COLOR_YELLOW));
            waddstr(wBorder, ") Open Orders");
          }
          waddstr(wBorder, " while");
          wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
          waddstr(wBorder, (string(" 1 ") + base + " = ").data());
          wattron(wBorder, A_BOLD);
          waddstr(wBorder, fairValue.data());
          wattroff(wBorder, A_BOLD);
          waddstr(wBorder, (string(" ") + quote).data());
          wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
          waddch(wBorder, (!gwConnected or !*gwConnected) ? ' ' : ':');
        }
        mvwaddch(wBorder, y-1, 0, ACS_LLCORNER);
        mvwaddstr(wBorder, 1, 2, string("|/-\\").substr(spinOrders, 1).data());
        move(yMaxLog-1, 2);
        wrefresh(wBorder);
        wrefresh(wLog);
      };
    private:
      function<void()> happyEnding = [&]() {
        quit();
        cout << '\n' << stamp();
      };
      void hotkeys() {
        hotkey = ::async(launch::async, [&] { return (mHotkey)wgetch(wBorder); });
      };
#if CAN_RESIZE
      function<void()> resize = [&]() {
        struct winsize ws;
        if (ioctl(0, TIOCGWINSZ, &ws) < 0
          or (ws.ws_row == getmaxy(wBorder)
          and ws.ws_col == getmaxx(wBorder))
        ) return;
        werase(wBorder);
        werase(wLog);
        if (ws.ws_row < 10) ws.ws_row = 10;
        if (ws.ws_col < 30) ws.ws_col = 30;
        wresize(wBorder, ws.ws_row, ws.ws_col);
        resizeterm(ws.ws_row, ws.ws_col);
        refresh();
      };
#endif
  };
}

#endif
