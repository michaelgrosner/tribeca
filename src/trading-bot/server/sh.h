#ifndef K_SH_H_
#define K_SH_H_

namespace K {
  class SH: public Screen {
    private:
      future<mHotkey> hotkey;
      unordered_map<mHotkey, function<void()>> hotFn;
      WINDOW *wBorder = nullptr,
             *wLog    = nullptr;
      int cursor = 0;
    protected:
      void load() {
        endingFn.insert(endingFn.begin(), [&]() {
          switchOff();
          clog << stamp();
        });
        gw->logger = [&](const string &prefix, const string &reason, const string &highlight) {
          if (reason.find("Error") != string::npos)
            logWar(prefix, reason);
          else log(prefix, reason, highlight);
        };
      };
      void waitData() {
        if (!options.num("latency")) return;
        gw->latency("HTTP read/write handshake", []() {
          options.handshake({
            {"gateway", gw->http}
          });
        });
        exit("1 HTTP connection done" + Ansi::r(COLOR_WHITE)
          + " (consider to repeat a few times this check)");
      };
      void run() {
        gw->askForCancelAll = &qp.cancelOrdersAuto;
        engine->monitor.unlock          = &gw->unlock;
        engine->monitor.product.minTick = &gw->minTick;
        engine->monitor.product.minSize = &gw->minSize;
        if (!options.num("naked"))
          switchOn();
        if (!mREST::inet.empty())
          log("CF", "Network Interface for outgoing traffic is", mREST::inet);
      };
    public:
      void printme(mToScreen *const data) {
        data->print = [&](const string &prefix, const string &reason) {
          log(prefix, reason);
        };
        data->focus = [&](const string &prefix, const string &reason, const string &highlight) {
          log(prefix, reason, highlight);
        };
        data->warn = [&](const string &prefix, const string &reason) {
          logWar(prefix, reason);
        };
        data->refresh = [&]() {
          refresh();
        };
      };
      void pressme(const mHotkey &ch, function<void()> fn) {
        if (!wBorder) return;
        if (hotFn.find(ch) != hotFn.end())
          error("SH", string("Too many handlers for \"") + (char)ch + "\" pressme event");
        hotFn[ch] = fn;
      };
      void waitForUser() {
        if (!hotkey.valid() or hotkey.wait_for(chrono::nanoseconds(0)) != future_status::ready) return;
        mHotkey ch = hotkey.get();
        if (hotFn.find(ch) != hotFn.end())
          hotFn.at(ch)();
        hotkeys();
      };
      const string stamp() {
        chrono::system_clock::time_point clock = Tclock;
        chrono::system_clock::duration t = clock.time_since_epoch();
        t -= chrono::duration_cast<chrono::seconds>(t);
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(t);
        t -= milliseconds;
        auto microseconds = chrono::duration_cast<chrono::microseconds>(t);
        stringstream microtime;
        microtime << setfill('0') << '.'
          << setw(3) << milliseconds.count()
          << setw(3) << microseconds.count();
        time_t tt = chrono::system_clock::to_time_t(clock);
        int len = options.num("naked") ? 15 : 9;
        char datetime[len];
        strftime(datetime, len, options.num("naked") ? "%m/%d %T" : "%T", localtime(&tt));
        if (!wBorder) return (Ansi::b(COLOR_GREEN) + (datetime + (Ansi::r(COLOR_GREEN) + microtime.str()))) + Ansi::b(COLOR_WHITE) + ' ';
        wattron(wLog, COLOR_PAIR(COLOR_GREEN));
        wattron(wLog, A_BOLD);
        wprintw(wLog, datetime);
        wattroff(wLog, A_BOLD);
        wprintw(wLog, microtime.str().data());
        wattroff(wLog, COLOR_PAIR(COLOR_GREEN));
        wprintw(wLog, " ");
        return "";
      };
      void logWar(const string &k, const string &s) {
        if (!wBorder) {
          cout << stamp() << k << Ansi::r(COLOR_RED) << " Warrrrning: " << Ansi::b(COLOR_RED) << s << '.' << Ansi::r(COLOR_WHITE) << endl;
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, k.data());
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, COLOR_PAIR(COLOR_RED));
        wprintw(wLog, " Warrrrning: ");
        wattroff(wLog, A_BOLD);
        wprintw(wLog, s.data());
        wprintw(wLog, ".");
        wattroff(wLog, COLOR_PAIR(COLOR_RED));
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wprintw(wLog, "\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
      void log(const string &prefix, const string &reason, const string &highlight = "") {
        unsigned int color = 0;
        if (reason.find("NG TRADE") != string::npos) {
          if (reason.find("BUY") != string::npos)
            color = 1;
          else if (reason.find("SELL") != string::npos)
            color = -1;
        }
        if (!wBorder) {
          cout << stamp() << prefix;
          if (color == 1)       cout << Ansi::r(COLOR_CYAN);
          else if (color == -1) cout << Ansi::r(COLOR_MAGENTA);
          else                  cout << Ansi::r(COLOR_WHITE);
          cout << ' ' << reason;
          if (!highlight.empty())
            cout << ' ' << Ansi::b(COLOR_YELLOW) << highlight;
          cout << Ansi::r(COLOR_WHITE) << ".\n";
          return;
        }
        wmove(wLog, getmaxy(wLog)-1, 0);
        stamp();
        wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        wattron(wLog, A_BOLD);
        wprintw(wLog, prefix.data());
        wattroff(wLog, A_BOLD);
        if (color == 1)       wattron(wLog, COLOR_PAIR(COLOR_CYAN));
        else if (color == -1) wattron(wLog, COLOR_PAIR(COLOR_MAGENTA));
        wprintw(wLog, (" " + reason).data());
        if (color == 1)       wattroff(wLog, COLOR_PAIR(COLOR_CYAN));
        else if (color == -1) wattroff(wLog, COLOR_PAIR(COLOR_MAGENTA));
        if (!highlight.empty()) {
          wprintw(wLog, " ");
          wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
          wattron(wLog, COLOR_PAIR(COLOR_YELLOW));
          wprintw(wLog, highlight.data());
          wattroff(wLog, COLOR_PAIR(COLOR_YELLOW));
          wattron(wLog, COLOR_PAIR(COLOR_WHITE));
        }
        wprintw(wLog, ".\n");
        wattroff(wLog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(wLog);
      };
    private:
      void hotkeys() {
        hotkey = ::async(launch::async, [&] {
          return (mHotkey)wgetch(wBorder);
        });
      };
      void switchOff() {
        if (!wBorder) return;
        beep();
        endwin();
        wBorder = nullptr;
      };
      void switchOn() {
        if (!(wBorder = initscr()))
          error("SH",
            "Unable to initialize ncurses, try to run in your terminal"
              "\"export TERM=xterm\", or use --naked argument"
          );
        if (Ansi::colorful) start_color();
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
        signal(SIGWINCH, [&]() {
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
        });
#endif
        refresh();
        hotkeys();
      };
      void refresh() {
        if (!wBorder) return;
        const vector<mOrder> openOrders = engine->orders.working(true);
        int lastcursor = cursor,
            y = getmaxy(wBorder),
            x = getmaxx(wBorder),
            yMaxLog = y - max((int)openOrders.size(), engine->broker.semaphore.paused() ? 0 : 2) - 1,
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
        for (const mOrder &it : openOrders) {
          wattron(wBorder, COLOR_PAIR(it.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
          mvwaddstr(wBorder, ++yOrders, 1, (((it.side == mSide::Bid ? "BID" : "ASK") + (" > "
            + mText::str8(it.quantity))) + ' ' + gw->base + " at price "
            + mText::str8(it.price) + ' ' + gw->quote + " (value "
            + mText::str8(abs(it.price * it.quantity)) + ' ' + gw->quote + ")"
          ).data());
          wattroff(wBorder, COLOR_PAIR(it.side == mSide::Bid ? COLOR_CYAN : COLOR_MAGENTA));
        }
        mvwaddch(wBorder, 0, 0, ACS_ULCORNER);
        mvwhline(wBorder, 0, 1, ACS_HLINE, max(80, x));
        mvwhline(wBorder, 1, 14, ' ', max(80, x)-14);
        mvwvline(wBorder, 1, 0, ACS_VLINE, yMaxLog);
        mvwvline(wBorder, yMaxLog, 0, ACS_LTEE, y);
        mvwaddch(wBorder, y, 0, ACS_BTEE);
        mvwaddch(wBorder, 0, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        string title1 = "   " + options.str("exchange");
        string title2 = " " + (options.num("port")
          ? "UI at " + mText::strL(client->protocol) + "://" + client->wtfismyip + ":" + options.str("port")
          : "headless"
        )  + ' ';
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, 13, title1.data());
        wattroff(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, 13+title1.length(), title2.data());
        mvwaddch(wBorder, 0, 14, 'K' | A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        mvwaddch(wBorder, 0, 13+title1.length()+title2.length(), ACS_LTEE);
        mvwaddch(wBorder, 0, x-26, ACS_RTEE);
        mvwaddstr(wBorder, 0, x-25, (string(" [   ]: ") + (engine->broker.semaphore.paused() ? "Start" : "Stop?") + ", [ ]: Quit!").data());
        mvwaddch(wBorder, 0, x-9, 'q' | A_BOLD);
        wattron(wBorder, A_BOLD);
        mvwaddstr(wBorder, 0, x-23, "ESC");
        wattroff(wBorder, A_BOLD);
        mvwaddch(wBorder, 0, 7, ACS_TTEE);
        mvwaddch(wBorder, 1, 7, ACS_LLCORNER);
        mvwhline(wBorder, 1, 8, ACS_HLINE, 4);
        mvwaddch(wBorder, 1, 12, ACS_RTEE);
        wattron(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        const string baseValue  = mText::str8(engine->wallet.base.value),
                     quoteValue = mText::str8(engine->wallet.quote.value);
        wattron(wBorder, A_BOLD);
        waddstr(wBorder, (" " + baseValue + ' ').data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, gw->base.data());
        wattroff(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        wattron(wBorder, COLOR_PAIR(COLOR_GREEN));
        waddstr(wBorder, " or ");
        wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
        wattron(wBorder, COLOR_PAIR(COLOR_CYAN));
        wattron(wBorder, A_BOLD);
        waddstr(wBorder, quoteValue.data());
        wattroff(wBorder, A_BOLD);
        waddstr(wBorder, (" " + gw->quote + ' ').data());
        wattroff(wBorder, COLOR_PAIR(COLOR_CYAN));
        size_t xLenValue = 14+baseValue.length()+quoteValue.length()+gw->base.length()+gw->quote.length()+7,
               xMaxValue = max(xLenValue+1, 18+title1.length()+title2.length());
        mvwaddch(wBorder, 0, xMaxValue, ACS_TTEE);
        mvwaddch(wBorder, 1, xMaxValue, ACS_LRCORNER);
        mvwhline(wBorder, 1, xLenValue, ACS_HLINE, xMaxValue - xLenValue);
        mvwaddch(wBorder, 1, xLenValue, ACS_LTEE);
        const int yPos = max(1, (y / 2) - 6),
                  baseAmount  = round(engine->wallet.base.amount  * 10 / engine->wallet.base.value),
                  baseHeld    = round(engine->wallet.base.held    * 10 / engine->wallet.base.value),
                  quoteAmount = round(engine->wallet.quote.amount * 10 / engine->wallet.quote.value),
                  quoteHeld   = round(engine->wallet.quote.held   * 10 / engine->wallet.quote.value);
        mvwvline(wBorder, yPos+1, x-3, ' ', 10);
        mvwvline(wBorder, yPos+1, x-4, ' ', 10);
        wattron(wBorder, COLOR_PAIR(COLOR_CYAN));
        mvwvline(wBorder, yPos+11-quoteAmount-quoteHeld, x-4, ACS_VLINE, quoteHeld);
        wattron(wBorder, A_BOLD);
        mvwvline(wBorder, yPos+11-quoteAmount, x-4, ' ' | A_REVERSE, quoteAmount);
        wattroff(wBorder, A_BOLD);
        wattroff(wBorder, COLOR_PAIR(COLOR_CYAN));
        wattron(wBorder, COLOR_PAIR(COLOR_MAGENTA));
        mvwvline(wBorder, yPos+11-baseAmount-baseHeld, x-3, ACS_VLINE, baseHeld);
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
        if (engine->broker.semaphore.offline()) {
          wattron(wBorder, COLOR_PAIR(COLOR_RED));
          wattron(wBorder, A_BOLD);
          waddstr(wBorder, "DISCONNECTED");
          wattroff(wBorder, A_BOLD);
          wattroff(wBorder, COLOR_PAIR(COLOR_RED));
          waddch(wBorder, ')');
        } else {
          if (engine->broker.semaphore.paused()) {
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
          waddstr(wBorder, (" 1 " + gw->base + " = ").data());
          wattron(wBorder, A_BOLD);
          waddstr(wBorder, mText::str8(engine->levels.fairValue).data());
          wattroff(wBorder, A_BOLD);
          waddstr(wBorder, (" " + gw->quote).data());
          wattroff(wBorder, COLOR_PAIR(COLOR_GREEN));
          waddch(wBorder, engine->broker.semaphore.paused() ? ' ' : ':');
        }
        mvwaddch(wBorder, y-1, 0, ACS_LLCORNER);
        mvwaddstr(wBorder, 1, 2, string("|/-\\").substr(engine->monitor.orders_60s % 4, 1).data());
        move(yMaxLog-1, 2);
        wrefresh(wBorder);
        wrefresh(wLog);
      };
  } sh;
}

#endif
