#ifndef K_BOTS_H_
#define K_BOTS_H_
//! \file
//! \brief Minimal user application framework.

namespace ฿ {
  string epilogue;

  //! \brief     Call all endingFn once and print a last log msg.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void exit(const string &reason = "", const bool &reboot = false) {
    epilogue = reason + string((reason.empty() or reason.back() == '.') ? 0 : 1, '.');
    raise(reboot ? SIGTERM : SIGQUIT);
  };

  class Ansi {
    public:
      static int colorful;
      static const string reset() {
          return colorful ? "\033[0m" : "";
      };
      static const string r(const int &color) {
          return colorful ? "\033[0;3" + to_string(color) + 'm' : "";
      };
      static const string b(const int &color) {
          return colorful ? "\033[1;3" + to_string(color) + 'm' : "";
      };
      static void default_colors() {
        if (colorful) start_color();
        use_default_colors();
        init_pair(COLOR_WHITE,   COLOR_WHITE,   COLOR_BLACK);
        init_pair(COLOR_GREEN,   COLOR_GREEN,   COLOR_BLACK);
        init_pair(COLOR_RED,     COLOR_RED,     COLOR_BLACK);
        init_pair(COLOR_YELLOW,  COLOR_YELLOW,  COLOR_BLACK);
        init_pair(COLOR_BLUE,    COLOR_BLUE,    COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_CYAN,    COLOR_CYAN,    COLOR_BLACK);
      };
  };

  int Ansi::colorful = 1;

  string Curl::inet;

  //! \brief     Call all endingFn once and print a last error log msg.
  //! \param[in] prefix Allows any string, if possible with a length of 2.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void error(const string &prefix, const string &reason, const bool &reboot = false) {
    if (reboot) this_thread::sleep_for(chrono::seconds(3));
    exit(prefix + Ansi::r(COLOR_RED) + " Errrror: " + Ansi::b(COLOR_RED) + reason, reboot);
  };

  class Hotkey {
    private:
      future<char> keylogger;
      unordered_map<char, function<void()>> hotFn;
    public:
      void hotkey(const char &ch, function<void()> fn) {
        if (!keylogger.valid()) return;
        if (hotFn.find(ch) != hotFn.end())
          error("SH", string("Too many handlers for \"") + ch + "\" hotkey event");
        hotFn[ch] = fn;
      };
      void wait_for_hotkey() {
        if (!keylogger.valid()
          or keylogger.wait_for(chrono::nanoseconds(0)) != future_status::ready
        ) return;
        const char ch = keylogger.get();
        if (hotFn.find(ch) != hotFn.end())
          hotFn.at(ch)();
        launch_keylogger();
      };
    protected:
      void legit_keylogger() {
        if (keylogger.valid())
          error("SH", string("Unable to launch another \"keylogger\" thread"));
        noecho();
        halfdelay(5);
        keypad(stdscr, true);
        launch_keylogger();
      };
    private:
      void launch_keylogger() {
        keylogger = ::async(launch::async, [&]() {
          int ch = ERR;
          while (ch == ERR and !hotFn.empty())
            ch = getch();
          return ch == ERR ? '\r' : (char)ch;
        });
      };
  };

  class Print {
    public:
      static WINDOW *stdlog;
      static void (*display)();
      static const bool windowed() {
        if (display) {
          if (stdlog)
            error("SH", "Unable to launch another window");
          if (!initscr())
            error("SH",
              "Unable to initialize ncurses, try to run in your terminal"
                "\"export TERM=xterm\", or use --naked argument"
            );
          Ansi::default_colors();
          stdlog = subwin(stdscr, getmaxy(stdscr)-4, getmaxx(stdscr)-2-6, 3, 2);
          scrollok(stdlog, true);
          idlok(stdlog, true);
          signal(SIGWINCH, [](const int sig) {
            endwin();
            refresh();
            clear();
          });
          repaint();
        }
        return display;
      };
      static void repaint() {
        if (display) display();
      };
      static const string stamp() {
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
        char datetime[15];
        strftime(datetime, 15, "%m/%d %T", localtime(&tt));
        if (!display) return Ansi::b(COLOR_GREEN) + datetime +
                             Ansi::r(COLOR_GREEN) + microtime.str()+
                             Ansi::b(COLOR_WHITE) + ' ';
        wattron(stdlog, COLOR_PAIR(COLOR_GREEN));
        wattron(stdlog, A_BOLD);
        wprintw(stdlog, datetime);
        wattroff(stdlog, A_BOLD);
        wprintw(stdlog, microtime.str().data());
        wattroff(stdlog, COLOR_PAIR(COLOR_GREEN));
        wprintw(stdlog, " ");
        return "";
      };
      static void log(const string &prefix, const string &reason, const string &highlight = "") {
        unsigned int color = 0;
        if (reason.find("NG TRADE") != string::npos) {
          if (reason.find("BUY") != string::npos)       color = 1;
          else if (reason.find("SELL") != string::npos) color = -1;
        }
        if (!display) {
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
        stamp();
        wattron(stdlog, COLOR_PAIR(COLOR_WHITE));
        wattron(stdlog, A_BOLD);
        wprintw(stdlog, prefix.data());
        wattroff(stdlog, A_BOLD);
        if (color == 1)       wattron(stdlog, COLOR_PAIR(COLOR_CYAN));
        else if (color == -1) wattron(stdlog, COLOR_PAIR(COLOR_MAGENTA));
        wprintw(stdlog, (" " + reason).data());
        if (color == 1)       wattroff(stdlog, COLOR_PAIR(COLOR_CYAN));
        else if (color == -1) wattroff(stdlog, COLOR_PAIR(COLOR_MAGENTA));
        if (!highlight.empty()) {
          wprintw(stdlog, " ");
          wattroff(stdlog, COLOR_PAIR(COLOR_WHITE));
          wattron(stdlog, COLOR_PAIR(COLOR_YELLOW));
          wprintw(stdlog, highlight.data());
          wattroff(stdlog, COLOR_PAIR(COLOR_YELLOW));
          wattron(stdlog, COLOR_PAIR(COLOR_WHITE));
        }
        wprintw(stdlog, ".\n");
        wattroff(stdlog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(stdlog);
      };
      static void logWar(const string &prefix, const string &reason) {
        if (!display) {
          cout << stamp()
               << prefix          << Ansi::r(COLOR_RED)
               << " Warrrrning: " << Ansi::b(COLOR_RED)
               << reason << '.'   << Ansi::r(COLOR_WHITE)
               << endl;
          return;
        }
        stamp();
        wattron(stdlog, COLOR_PAIR(COLOR_WHITE));
        wattron(stdlog, A_BOLD);
        wprintw(stdlog, prefix.data());
        wattroff(stdlog, COLOR_PAIR(COLOR_WHITE));
        wattron(stdlog, COLOR_PAIR(COLOR_RED));
        wprintw(stdlog, " Warrrrning: ");
        wattroff(stdlog, A_BOLD);
        wprintw(stdlog, reason.data());
        wprintw(stdlog, ".");
        wattroff(stdlog, COLOR_PAIR(COLOR_RED));
        wattron(stdlog, COLOR_PAIR(COLOR_WHITE));
        wprintw(stdlog, "\n");
        wattroff(stdlog, COLOR_PAIR(COLOR_WHITE));
        wrefresh(stdlog);
      };
  };

  void (*Print::display)() = nullptr;

  WINDOW *Print::stdlog = nullptr;

  vector<function<void()>> happyEndingFn, endingFn = { []() {
    if (Print::display) {
      Print::display = nullptr;
      beep();
      endwin();
    }
    clog << Print::stamp()
         << epilogue
         << string(epilogue.empty() ? 0 : 1, '\n');
  } };

  const mClock rollout = Tstamp;

  class Rollout {
    public:
      Rollout(/* KMxTWEpb9ig */) {
        clog << Ansi::b(COLOR_GREEN) << K_SOURCE
             << Ansi::r(COLOR_GREEN) << ' ' << K_BUILD << ' ' << K_STAMP << ".\n";
        const string mods = changelog();
        const int commits = count(mods.begin(), mods.end(), '\n');
        clog << Ansi::b(COLOR_GREEN) << K_0_DAY << Ansi::r(COLOR_GREEN) << ' '
             << (commits
                 ? '-' + to_string(commits) + "commit"
                   + string(commits == 1 ? 0 : 1, 's') + '.'
                 : "(0day)"
                )
#ifndef NDEBUG
            << " with DEBUG MODE enabled"
#endif
            << ".\n" << Ansi::r(COLOR_YELLOW) << mods << Ansi::reset();
      };
    protected:
      static const string changelog() {
        string mods;
        const json diff = Curl::xfer("https://api.github.com/repos/ctubio/Krypto-trading-bot"
                                      "/compare/" + string(K_0_GIT) + "...HEAD", 4L);
        if (diff.value("ahead_by", 0)
          and diff.find("commits") != diff.end()
          and diff.at("commits").is_array()
        ) for (const json &it : diff.at("commits"))
          mods += it.value("/commit/author/date"_json_pointer, "").substr(0, 10) + " "
                + it.value("/commit/author/date"_json_pointer, "").substr(11, 8)
                + " (" + it.value("sha", "").substr(0, 7) + ") "
                + it.value("/commit/message"_json_pointer, "").substr(0,
                  it.value("/commit/message"_json_pointer, "").find("\n\n") + 1
                );
        return mods;
      };
  };

  class Ending: public Rollout {
    public:
      Ending() {
        signal(SIGINT, [](const int sig) {
          clog << '\n';
          raise(SIGQUIT);
        });
        signal(SIGQUIT, die);
        signal(SIGTERM, err);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
#ifndef _WIN32
        signal(SIGUSR1, wtf);
#endif
      };
    private:
      static void halt(const int code) {
        endingFn.swap(happyEndingFn);
        for (function<void()> &it : happyEndingFn) it();
        Ansi::colorful = 1;
        clog << Ansi::b(COLOR_GREEN) << 'K'
             << Ansi::r(COLOR_GREEN) << " exit code "
             << Ansi::b(COLOR_GREEN) << code
             << Ansi::r(COLOR_GREEN) << '.'
             << Ansi::reset() << '\n';
        EXIT(code);
      };
      static void die(const int sig) {
        if (epilogue.empty())
          epilogue = "Excellent decision! "
                   + Curl::xfer("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
                       .value("/value/joke"_json_pointer, "let's plant a tree instead..");
        halt(
          epilogue.find("Errrror") == string::npos
            ? EXIT_SUCCESS
            : EXIT_FAILURE
        );
      };
      static void err(const int sig) {
        if (epilogue.empty()) epilogue = "Unknown error, no joke.";
        halt(EXIT_FAILURE);
      };
      static void wtf(const int sig) {
        epilogue = Ansi::r(COLOR_CYAN) + "Errrror: " + strsignal(sig) + ' ';
        const string mods = changelog();
        if (mods.empty()) {
          epilogue += "(Three-Headed Monkey found):\n";
          if (gw)
            epilogue += "- exchange: " + gw->exchange              + '\n'
                      + "- currency: " + (gw->symbol.empty()
                                           ? gw->base + " .. " + gw->quote
                                           : gw->symbol)           + '\n';
          epilogue += "- lastbeat: " + to_string(Tstamp - rollout) + '\n'
                    + "- binbuild: " + string(K_SOURCE)            + ' '
                                     + string(K_BUILD)             + '\n'
#ifndef _WIN32
            + "- tracelog: " + '\n';
          void *k[69];
          size_t jumps = backtrace(k, 69);
          char **trace = backtrace_symbols(k, jumps);
          for (size_t i = 0; i < jumps; i++)
            epilogue += string(trace[i]) + '\n';
          free(trace)
#endif
          ;
          epilogue += '\n' + Ansi::b(COLOR_RED) + "Yikes!" + Ansi::r(COLOR_RED)
            + '\n' + "please copy and paste the error above into a new github issue (noworry for duplicates)."
            + '\n' + "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
            + '\n' + '\n';
        } else
          epilogue += string("(deprecated K version found).") + '\n'
            + '\n' + Ansi::b(COLOR_YELLOW) + "Hint!" + Ansi::r(COLOR_YELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + mods
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        halt(EXIT_FAILURE);
      };
  };

  //! \brief Placeholder to avoid spaghetti codes.
  //! - Walks through minimal runtime steps when wait() is called.
  //! - Adds end() into endingFn to be called on exit().
  //! - Connects to gateway when ready.
  class Klass {
    protected:
      virtual void load        ()    {};
      virtual void waitData    ()   {};
      virtual void waitWebAdmin()  {};
      virtual void waitSysAdmin(){};
      virtual void waitTime    ()  {};
      virtual void run         ()   {};
      virtual void end         ()   {};
    public:
      void wait() {
        load();
        waitData();
        waitWebAdmin();
        waitSysAdmin();
        waitTime();
        endingFn.push_back([&]() {
          end();
        });
        run();
        if (gw->ready()) gw->run();
      };
  };

  struct Argument {
   const string  name;
   const string  defined_value;
   const char   *default_value;
   const string  help;
  };

  class Option {
    public:
      pair<vector<Argument>, function<void(
        unordered_map<string, string> &,
        unordered_map<string, int>    &,
        unordered_map<string, double> &
      )>> arguments;
    private:
      unordered_map<string, string> optstr;
      unordered_map<string, int>    optint;
      unordered_map<string, double> optdec;
    public:
      const string str(const string &name) const {
        return optstr.find(name) != optstr.end()
          ? optstr.at(name)
          : (optint.find(name) != optint.end()
              ? to_string(num(name))
              : Text::str8(dec(name))
          );
      };
      const int num(const string &name) const {
        return optint.at(name);
      };
      const double dec(const string &name) const {
        return optdec.at(name);
      };
      void main(int argc, char** argv) {
        optint["naked"] = !Print::display;
        vector<Argument> long_options = {
          {"help",         "h",      0,        "show this help and quit"},
          {"version",      "v",      0,        "show current build version and quit"},
          {"latency",      "1",      0,        "check current HTTP latency (not from WS) and quit"}
        };
        if (Print::display)
          long_options.push_back(
            {"naked",        "1",      0,        "do not display CLI, print output to stdout instead"}
          );
        for (const Argument &it : (vector<Argument>){
          {"interface",    "IP",     "",       "set IP to bind as outgoing network interface,"
                                               "\n" "default IP is the system default network interface"},
          {"exchange",     "NAME",   "NULL",   "set exchange NAME for trading, mandatory one of:"
                                               "\n" "'COINBASE', 'BITFINEX', 'ETHFINEX', 'HITBTC',"
                                               "\n" "'KRAKEN', 'FCOIN', 'KORBIT' , 'POLONIEX' or 'NULL'"},
          {"currency",     "PAIR",   "NULL",   "set currency PAIR for trading, use format"
                                               "\n" "with '/' separator, like 'BTC/EUR'"},
          {"apikey",       "WORD",   "NULL",   "set (never share!) WORD as api key for trading, mandatory"},
          {"secret",       "WORD",   "NULL",   "set (never share!) WORD as api secret for trading, mandatory"},
          {"passphrase",   "WORD",   "NULL",   "set (never share!) WORD as api passphrase for trading,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"username",     "WORD",   "NULL",   "set (never share!) WORD as api username for trading,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"http",         "URL",    "",       "set URL of alernative HTTPS api endpoint for trading"},
          {"wss",          "URL",    "",       "set URL of alernative WSS api endpoint for trading"},
          {"fix",          "URL",    "",       "set URL of alernative FIX api endpoint for trading"},
          {"market-limit", "NUMBER", "321",    "set NUMBER of maximum price levels for the orderbook,"
                                               "\n" "default NUMBER is '321' and the minimum is '15'."
                                               "\n" "locked bots smells like '--market-limit=3' spirit"}
        }) long_options.push_back(it);
        for (const Argument &it : arguments.first)
          long_options.push_back(it);
        arguments.first.clear();
        for (const Argument &it : (vector<Argument>){
          {"debug-secret", "1",      0,        "print (never share!) secret inputs and outputs"},
          {"debug",        "1",      0,        "print detailed output about all the (previous) things!"},
          {"colors",       "1",      0,        "print highlighted output"},
          {"title",        "WORD",   K_SOURCE, "set WORD to allow admins to identify different bots"},
          {"free-version", "1",      0,        "work with all market levels but slowdown 7 seconds"}
        }) long_options.push_back(it);
        int index = 1714;
        vector<option> opt_long = { {0, 0, 0, 0} };
        for (const Argument &it : long_options) {
          if     (!it.default_value)             optint[it.name] = 0;
          else if (it.defined_value == "NUMBER") optint[it.name] = stoi(it.default_value);
          else if (it.defined_value == "AMOUNT") optdec[it.name] = stod(it.default_value);
          else                                   optstr[it.name] =      it.default_value;
          opt_long.insert(opt_long.end()-1, {
            it.name.data(),
            it.default_value
              ? required_argument
              : no_argument,
            it.default_value or it.defined_value.at(0) > '>'
              ? nullptr
              : &optint[it.name],
            it.default_value
              ? index++
              : (it.defined_value.at(0) > '>'
                ? (int)it.defined_value.at(0)
                : stoi(it.defined_value)
              )
          });
        }
        int k = 0;
        while (++k)
          switch (k = getopt_long(argc, argv, "hv", (option*)&opt_long[0], &index)) {
            case -1 :
            case  0 : break;
            case 'h': help(long_options);
            case '?':
            case 'v': EXIT(EXIT_SUCCESS);
            default : {
              const string name(opt_long.at(index).name);
              if      (optint.find(name) != optint.end()) optint[name] =   stoi(optarg);
              else if (optdec.find(name) != optdec.end()) optdec[name] =   stod(optarg);
              else                                        optstr[name] = string(optarg);
            }
          }
        if (optind < argc) {
          string argerr = "Unhandled argument option(s):";
          while(optind < argc) argerr += string(" ") + argv[optind++];
          error("CF", argerr);
        }
        tidy();
        gateway();
        curl_global_init(CURL_GLOBAL_ALL);
        Curl::inet = str("interface");
        Ansi::colorful = num("colors");
        if (num("naked")) Print::display = nullptr;
      };
    private:
      void tidy() {
        if (optstr["currency"].find("/") == string::npos or optstr["currency"].length() < 3)
          error("CF", "Invalid --currency value; must be in the format of BASE/QUOTE, like BTC/EUR");
        if (optstr["exchange"].empty())
          error("CF", "Invalid --exchange value; the config file may have errors (there are extra spaces or double defined variables?)");
        optstr["exchange"] = Text::strU(optstr["exchange"]);
        optstr["currency"] = Text::strU(optstr["currency"]);
        optstr["base"]  = optstr["currency"].substr(0, optstr["currency"].find("/"));
        optstr["quote"] = optstr["currency"].substr(1+ optstr["currency"].find("/"));
        optint["market-limit"] = max(15, optint["market-limit"]);
        if (optint["debug"])
          optint["debug-secret"] = 1;
#ifndef _WIN32
        if (optint["latency"] or optint["debug-secret"])
#endif
          optint["naked"] = 1;
        if (arguments.second) {
          arguments.second(
            optstr,
            optint,
            optdec
          );
          arguments.second = nullptr;
        }
      };
      void gateway() {
        if (!(gw = Gw::new_Gw(str("exchange"))))
          error("CF",
            "Unable to configure a valid gateway using --exchange="
              + str("exchange") + " argument"
          );
        if (!str("http").empty()) gw->http = str("http");
        if (!str("wss").empty())  gw->ws   = str("wss");
        if (!str("fix").empty())  gw->fix  = str("fix");
        gw->exchange = str("exchange");
        gw->base     = str("base");
        gw->quote    = str("quote");
        gw->apikey   = str("apikey");
        gw->secret   = str("secret");
        gw->user     = str("username");
        gw->pass     = str("passphrase");
        gw->maxLevel = num("market-limit");
        gw->debug    = num("debug-secret");
        gw->version  = num("free-version");
        gw->printer = [&](const string &prefix, const string &reason, const string &highlight) {
          if (reason.find("Error") != string::npos)
            Print::logWar(prefix, reason);
          else Print::log(prefix, reason, highlight);
        };
      };
      void help(const vector<Argument> &long_options) {
        const vector<string> stamp = {
          " \\__/  \\__/ ", " | (   .    ", "  __   \\__/ ",
          " /  \\__/  \\ ", " |  `.  `.  ", " /  \\       ",
          " \\__/  \\__/ ", " |    )   ) ", " \\__/   __  ",
          " /  \\__/  \\ ", " |  ,'  ,'  ", "       /  \\ "
        };
              unsigned int y = Tstamp;
        const unsigned int x = !(y % 2)
                             + !(y % 21);
        clog
          << Ansi::r(COLOR_GREEN) << PERMISSIVE_analpaper_SOFTWARE_LICENSE << '\n'
          << Ansi::r(COLOR_GREEN) << "  questions: " << Ansi::r(COLOR_YELLOW) << "https://earn.com/analpaper/" << '\n'
          << Ansi::b(COLOR_GREEN) << "K" << Ansi::r(COLOR_GREEN) << " bugkiller: " << Ansi::r(COLOR_YELLOW) << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << '\n'
          << Ansi::r(COLOR_GREEN) << "  downloads: " << Ansi::r(COLOR_YELLOW) << "ssh://git@github.com/ctubio/Krypto-trading-bot" << '\n'
          << Ansi::b(COLOR_WHITE) << stamp.at(((++y%4)*3)+x) << "Usage:" << Ansi::b(COLOR_YELLOW) << " " << K_SOURCE << " [arguments]" << '\n'
          << Ansi::b(COLOR_WHITE) << stamp.at(((++y%4)*3)+x) << "[arguments]:";
        for (const Argument &it : long_options) {
          string usage = it.help;
          string::size_type n = 0;
          while ((n = usage.find('\n', n + 1)) != string::npos)
            usage.insert(n + 1, 28, ' ');
          const string example = "--" + it.name + (it.default_value ? "=" + it.defined_value : "");
          usage = '\n' + (
            (!it.default_value and it.defined_value.at(0) > '>')
              ? "-" + it.defined_value + ", "
              : "    "
          ) + example + string(22 - example.length(), ' ')
            + "- " + usage;
          n = 0;
          do usage.insert(n + 1, Ansi::b(COLOR_WHITE) + stamp.at(((++y%4)*3)+x) + Ansi::r(COLOR_WHITE));
          while ((n = usage.find('\n', n + 1)) != string::npos);
          clog << usage << '.';
        }
        clog << '\n'
          << Ansi::r(COLOR_GREEN) << "  more help: " << Ansi::r(COLOR_YELLOW) << "https://github.com/ctubio/Krypto-trading-bot/blob/master/doc/MANUAL.md" << '\n'
          << Ansi::b(COLOR_GREEN) << "K" << Ansi::r(COLOR_GREEN) << " questions: " << Ansi::r(COLOR_YELLOW) << "irc://irc.freenode.net:6667/#tradingBot" << '\n'
          << Ansi::r(COLOR_GREEN) << "  home page: " << Ansi::r(COLOR_YELLOW) << "https://ca.rles-tub.io./trades" << '\n'
          << Ansi::reset();
      };
  };

  class KryptoNinja: public Klass,
                     public Ending,
                     public Hotkey {
    public:
      Option option;
    public:
      KryptoNinja *const main(int argc, char** argv) {
        option.main(argc, argv);
        if (Print::windowed()) legit_keylogger();
        if (option.num("latency")) {
          gw->latency("HTTP read/write handshake", [&]() {
            handshake({
              {"gateway", gw->http}
            });
          });
          exit("1 HTTP connection done" + Ansi::r(COLOR_WHITE)
            + " (consider to repeat a few times this check)");
        }
        return this;
      };
      void wait(const vector<Klass*> &k = {}) {
        if (k.empty()) Klass::wait();
        else for (Klass *const it : k) it->wait();
      };
      void handshake(const vector<pair<string, string>> &notes = {}) {
        const json reply = gw->handshake();
        if (!gw->randId or gw->symbol.empty())
          error("GW", "Incomplete handshake aborted");
        if (!gw->minTick or !gw->minSize)
          error("GW", "Unable to fetch data from " + gw->exchange
            + " for symbol \"" + gw->symbol + "\", possible error message: "
            + reply.dump());
        gw->info(notes);
      };
  };
}

#endif
