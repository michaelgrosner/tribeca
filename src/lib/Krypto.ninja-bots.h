#ifndef K_BOTS_H_
#define K_BOTS_H_
//! \file
//! \brief Minimal user application framework.

namespace ₿ {
  string epilogue, epitaph;

  enum class mPortal: unsigned char {
    Hello = '=',
    Kiss  = '-'
  };
  enum class mMatter: unsigned char {
    FairValue            = 'a', Quote                = 'b', ActiveSubscription = 'c', Connectivity       = 'd',
    MarketData           = 'e', QuotingParameters    = 'f', SafetySettings     = 'g', Product            = 'h',
    OrderStatusReports   = 'i', ProductAdvertisement = 'j', ApplicationState   = 'k', EWMAStats          = 'l',
    STDEVStats           = 'm', Position             = 'n', Profit             = 'o', SubmitNewOrder     = 'p',
    CancelOrder          = 'q', MarketTrade          = 'r', Trades             = 's', ExternalValuation  = 't',
    QuoteStatus          = 'u', TargetBasePosition   = 'v', TradeSafetyValue   = 'w', CancelAllOrders    = 'x',
    CleanAllClosedTrades = 'y', CleanAllTrades       = 'z', CleanTrade         = 'A',
    WalletChart          = 'C', MarketChart          = 'D', Notepad            = 'E',
                                MarketDataLongTerm   = 'H'
  };

  //! \brief     Call all endingFn once and print a last log msg.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void exit(const string &reason = "", const bool &reboot = false) {
    epilogue = reason + string((reason.empty() or reason.back() == '.') ? 0 : 1, '.');
    raise(reboot ? SIGTERM : SIGQUIT);
  };

  const char *Curl::inet = nullptr;
  mutex Curl::waiting_reply;

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
    protected:
      function<const bool()> legit_keylogger() {
        if (keylogger.valid())
          error("SH", string("Unable to launch another \"keylogger\" thread"));
        noecho();
        halfdelay(5);
        keypad(stdscr, true);
        launch_keylogger();
        return [&]() {
          return wait_for_keylog();
        };
      };
    private:
      const bool wait_for_keylog() {
        if (keylogger.valid()
          and keylogger.wait_for(chrono::nanoseconds(0)) == future_status::ready
        ) {
          const char ch = keylogger.get();
          if (hotFn.find(ch) != hotFn.end())
            hotFn.at(ch)();
          launch_keylogger();
        }
        return false;
      };
      void launch_keylogger() {
        keylogger = ::async(launch::async, [&]() {
          int ch = ERR;
          while (ch == ERR and !hotFn.empty())
            ch = getch();
          return ch == ERR ? '\r' : (char)ch;
        });
      };
  };

  struct Margin {
    unsigned int top;
    unsigned int right;
    unsigned int bottom;
    unsigned int left;
  };

  class Print {
    public:
      static WINDOW *stdlog;
      static Margin margin;
      static void (*display)();
      static const bool windowed() {
        if (!display) return false;
        if (stdlog)
          error("SH", "Unable to print another window");
        if (!initscr())
          error("SH",
            "Unable to initialize ncurses, try to run in your terminal"
              "\"export TERM=xterm\", or use --naked argument"
          );
        Ansi::default_colors();
        if (margin.top != ANY_NUM) {
          stdlog = subwin(
            stdscr,
            getmaxy(stdscr) - margin.bottom - margin.top,
            getmaxx(stdscr) - margin.left - margin.right,
            margin.top,
            margin.left
          );
          scrollok(stdlog, true);
          idlok(stdlog, true);
        }
        signal(SIGWINCH, [](const int sig) {
          endwin();
          refresh();
          clear();
        });
        repaint();
        return true;
      };
      static void repaint() {
        if (!display) return;
        display();
        wrefresh(stdscr);
        if (stdlog) {
          wmove(stdlog, getmaxy(stdlog) - 1, 0);
          wrefresh(stdlog);
        }
      };
      static const string stamp() {
        chrono::system_clock::time_point clock = chrono::system_clock::now();
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
        if (stdlog) {
          wattron(stdlog, COLOR_PAIR(COLOR_GREEN));
          wattron(stdlog, A_BOLD);
          wprintw(stdlog, datetime);
          wattroff(stdlog, A_BOLD);
          wprintw(stdlog, microtime.str().data());
          wattroff(stdlog, COLOR_PAIR(COLOR_GREEN));
          wprintw(stdlog, " ");
        }
        return "";
      };
      static void log(const string &prefix, const string &reason, const string &highlight = "") {
        unsigned int color = 0;
        if (reason.find("NG TRADE") != string::npos) {
          if (reason.find("BUY") != string::npos)       color = 1;
          else if (reason.find("SELL") != string::npos) color = -1;
        }
        if (!display) {
          clog << stamp() << prefix;
          if (color == 1)       clog << Ansi::r(COLOR_CYAN);
          else if (color == -1) clog << Ansi::r(COLOR_MAGENTA);
          else                  clog << Ansi::r(COLOR_WHITE);
          clog << ' ' << reason;
          if (!highlight.empty())
            clog << ' ' << Ansi::b(COLOR_YELLOW) << highlight;
          clog << Ansi::r(COLOR_WHITE) << ".\n";
          return;
        }
        if (!stdlog) return;
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
          clog << stamp()
               << prefix          << Ansi::r(COLOR_RED)
               << " Warrrrning: " << Ansi::b(COLOR_RED)
               << reason << '.'   << Ansi::r(COLOR_WHITE)
               << endl;
          return;
        }
        if (!stdlog) return;
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

  WINDOW *Print::stdlog = nullptr;

  Margin Print::margin = {ANY_NUM, 0, 0, 0};

  void (*Print::display)() = nullptr;

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
    private:
      static vector<function<void()>> endingFn;
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
      void ending(const function<void()> &fn) {
        endingFn.push_back(fn);
      };
    private:
      static void halt(const int code) {
        vector<function<void()>> happyEndingFn;
        endingFn.swap(happyEndingFn);
        for (const auto &it : happyEndingFn) it();
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
          epilogue += "(Three-Headed Monkey found):\n"                  + epitaph
            + "- lastbeat: " + to_string((float)clock()/CLOCKS_PER_SEC) + '\n'
            + "- binbuild: " + string(K_SOURCE)                         + ' '
                             + string(K_BUILD)                          + '\n'
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

  vector<function<void()>> Ending::endingFn = { []() {
    if (Print::display) {
      Print::display = nullptr;
      beep();
      endwin();
    }
    clog << Print::stamp()
         << epilogue
         << string(epilogue.empty() ? 0 : 1, '\n');
  } };

  struct Argument {
   const string  name;
   const string  defined_value;
   const char   *default_value;
   const string  help;
  };

  class Option {
    protected:
      bool databases = false;
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
              : to_string(dec(name))
            );
      };
      const int num(const string &name) const {
#ifndef NDEBUG
        if (optint.find(name) == optint.end()) return 0;
#endif
        return optint.at(name);
      };
      const double dec(const string &name) const {
#ifndef NDEBUG
        if (optdec.find(name) == optdec.end()) return 0;
#endif
        return optdec.at(name);
      };
      const unsigned int memSize() const {
#ifdef _WIN32
        return 0;
#else
        struct rusage ru;
        return getrusage(RUSAGE_SELF, &ru) ? 0 : ru.ru_maxrss * 1e+3;
#endif
      };
      const unsigned int dbSize() const {
        if (!databases or str("database") == ":memory:") return 0;
        struct stat st;
        return stat(str("database").data(), &st) ? 0 : st.st_size;
      };
    protected:
      void main(int argc, char** argv) {
        optint["naked"] = !Print::display;
        vector<Argument> long_options = {
          {"help",         "h",      nullptr,  "show this help and quit"},
          {"version",      "v",      nullptr,  "show current build version and quit"},
          {"latency",      "1",      nullptr,  "check current HTTP latency (not from WS) and quit"}
        };
        if (Print::display) long_options.push_back(
          {"naked",        "1",      nullptr,  "do not display CLI, print output to stdout instead"}
        );
        if (databases) long_options.push_back(
          {"database",     "FILE",   "",       "set alternative PATH to database filename,"
                                               "\n" "default PATH is '/var/lib/K/db/K-*.db',"
                                               "\n" "or use ':memory:' (see sqlite.org/inmemorydb.html)"}
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
          {"dustybot",     "1",      nullptr,  "do not automatically cancel all orders on exit"},
          {"market-limit", "NUMBER", "321",    "set NUMBER of maximum price levels for the orderbook,"
                                               "\n" "default NUMBER is '321' and the minimum is '15'."
                                               "\n" "locked bots smells like '--market-limit=3' spirit"}
        }) long_options.push_back(it);
        for (const Argument &it : arguments.first)
          long_options.push_back(it);
        arguments.first.clear();
        for (const Argument &it : (vector<Argument>){
          {"debug-secret", "1",      nullptr,  "print (never share!) secret inputs and outputs"},
          {"debug",        "1",      nullptr,  "print detailed output about all the (previous) things!"},
          {"colors",       "1",      nullptr,  "print highlighted output"},
          {"title",        "WORD",   K_SOURCE, "set WORD to allow admins to identify different bots"},
          {"free-version", "1",      nullptr,  "work with all market levels but slowdown 7 seconds"}
        }) long_options.push_back(it);
        int index = ANY_NUM;
        vector<option> opt_long = { {nullptr, 0, nullptr, 0} };
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
        if (!str("interface").empty())
          Curl::inet = strdup(str("interface").data());
        if (num("naked")) Print::display = nullptr;
        Ansi::colorful = num("colors");
        if (arguments.second) {
          arguments.second(
            optstr,
            optint,
            optdec
          );
          arguments.second = nullptr;
        }
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
        if (databases) {
          optstr["diskdata"] = "";
          if (optstr["database"].empty() or optstr["database"] == ":memory:")
            (optstr["database"] == ":memory:"
              ? optstr["diskdata"]
              : optstr["database"]
            ) = ("/var/lib/K/db/" K_SOURCE)
              + ('.' + optstr["exchange"])
              +  '.' + optstr["base"]
              +  '.' + optstr["quote"]
              +  '.' + "db";
        }
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
          << Ansi::b(COLOR_WHITE) << stamp.at(((++y%4)*3)+x) << "Usage:" << Ansi::b(COLOR_YELLOW) << " " << K_SOURCE << " [arguments]" << '\n';
        clog
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

  class Socket {
    public:
      string wtfismyip = "localhost";
    protected:
      uWS::Hub *socket = nullptr;
      vector<uWS::Group<uWS::CLIENT>*> gw_clients;
      vector<uWS::Group<uWS::SERVER>*> ui_servers;
    public:
      uWS::Group<uWS::SERVER> *listen(
               string &protocol,
        const     int &port,
        const    bool &ssl,
        const  string &crt,
        const  string &key,
        const function<const string(
          const string&,
          const string&,
          const string&
        )>            &httpServer = nullptr,
        const function<const bool(
          const   bool&,
          const string&
        )>            &wsServer   = nullptr,
        const function<const string(
                string,
          const string&
        )>            &wsMessage  = nullptr
      ) {
        auto ui_server = socket->createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
        if (!ui_server) return nullptr;
        SSL_CTX *ctx = sslContext(ssl, crt, key);
        if (!socket->listen(Curl::inet, port, uS::TLS::Context(ctx), 0, ui_server))
          return nullptr;
        protocol += string(ctx ? 1 : 0, 'S');
        if (httpServer)
          ui_server->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
            if (req.getMethod() != uWS::HttpMethod::METHOD_GET) return;
            const string response = httpServer(
              req.getUrl().toString(),
              req.getHeader("authorization").toString(),
              cleanAddress(res->getHttpSocket()->getAddress().address)
            );
            if (!response.empty())
              res->write(response.data(), response.length());
          });
        if (wsServer) {
          ui_server->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
            if (!wsServer(true, cleanAddress(webSocket->getAddress().address)))
              webSocket->close();
          });
          ui_server->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
            wsServer(false, cleanAddress(webSocket->getAddress().address));
          });
        }
        if (wsMessage)
          ui_server->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
            if (length < 2) return;
            const string response = wsMessage(
              string(message, length),
              cleanAddress(webSocket->getAddress().address)
            );
            if (!response.empty())
              webSocket->send(response.data(), response.substr(0, 2) == "PK"
                                                 ? uWS::OpCode::BINARY
                                                 : uWS::OpCode::TEXT);
          });
        Print::log("UI", "ready at", Text::strL(protocol) + "://" + wtfismyip + ":" + to_string(port));
        ui_servers.push_back(ui_server);
        return ui_server;
      };
    protected:
      uWS::Group<uWS::CLIENT> *bind() {
        gw_clients.push_back(socket->createGroup<uWS::CLIENT>());
        return gw_clients.back();
      };
    private:
      SSL_CTX *sslContext(const bool &ssl, const string &crt, const string &key) {
        SSL_CTX *ctx = nullptr;
        if (ssl and (ctx = SSL_CTX_new(SSLv23_server_method()))) {
          SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
          if (crt.empty() or key.empty()) {
            if (!crt.empty())
              Print::logWar("UI", "Ignored .crt file because .key file is missing");
            if (!key.empty())
              Print::logWar("UI", "Ignored .key file because .crt file is missing");
            Print::logWar("UI", "Connected web clients will enjoy unsecure SSL encryption..\n"
              "(because the private key is visible in the source!). See --help argument to setup your own SSL");
            if (!SSL_CTX_use_certificate(ctx,
              PEM_read_bio_X509(BIO_new_mem_buf((void*)
                "-----BEGIN CERTIFICATE-----"                                      "\n"
                "MIICATCCAWoCCQCiyDyPL5ov3zANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB" "\n"
                "VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0" "\n"
                "cyBQdHkgTHRkMB4XDTE2MTIyMjIxMDMyNVoXDTE3MTIyMjIxMDMyNVowRTELMAkG" "\n"
                "A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0" "\n"
                "IFdpZGdpdHMgUHR5IEx0ZDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAunyx" "\n"
                "1lNsHkMmCa24Ns9xgJAwV3A6/Jg/S5jPCETmjPRMXqAp89fShZxN2b/2FVtU7q/N" "\n"
                "EtNpPyEhfAhPwYrkHCtip/RmZ/b6qY2Cx6otFIsuwO8aUV27CetpoM8TAQSuufcS" "\n"
                "jcZD9pCAa9GM/yWeqc45su9qBBmLnAKYuYUeDQUCAwEAATANBgkqhkiG9w0BAQsF" "\n"
                "AAOBgQAeZo4zCfnq5/6gFzoNDKg8DayoMnCtbxM6RkJ8b/MIZT5p6P7OcKNJmi1o" "\n"
                "XD2evdxNrY0ObQ32dpiLqSS1JWL8bPqloGJBNkSPi3I+eBoJSE7/7HOroLNbp6nS" "\n"
                "aaec6n+OlGhhjxn0DzYiYsVBUsokKSEJmHzoLHo3ZestTTqUwg=="             "\n"
                "-----END CERTIFICATE-----"                                        "\n"
              , -1), nullptr, nullptr, nullptr
            )) or !SSL_CTX_use_RSAPrivateKey(ctx,
              PEM_read_bio_RSAPrivateKey(BIO_new_mem_buf((void*)
                "-----BEGIN RSA PRIVATE KEY-----"                                  "\n"
                "MIICXAIBAAKBgQC6fLHWU2weQyYJrbg2z3GAkDBXcDr8mD9LmM8IROaM9ExeoCnz" "\n"
                "19KFnE3Zv/YVW1Tur80S02k/ISF8CE/BiuQcK2Kn9GZn9vqpjYLHqi0Uiy7A7xpR" "\n"
                "XbsJ62mgzxMBBK659xKNxkP2kIBr0Yz/JZ6pzjmy72oEGYucApi5hR4NBQIDAQAB" "\n"
                "AoGBAJi9OrbtOreKjeQNebzCqRcAgeeLz3RFiknzjVYbgK1gBhDWo6XJVe8C9yxq" "\n"
                "sjYJyQV5zcAmkaQYEaHR+OjvRiZ4UmXbItukOD+dnq7xs69n3w54FfANjkurdL2M" "\n"
                "fPAQm/GJT4TSBDIr7eJQPOrork9uxQStwADTqvklVlKm2YldAkEA80ZYaLrGOBbz" "\n"
                "5871ewKxtVJNCCmXdYUwq7nI/lqsLBZnB+wiwnQ+3tgfi4YoUoTnv0hIIwkyLYl9" "\n"
                "Z2wqensf6wJBAMQ96gUGnIcYJzknB5CYDNQalcvvTx7tLtgRXDf47bQJ3X/Q5k/t" "\n"
                "yDlByUBqvYVShXWs+d4ynNKLze/w18H8Os8CQBYFDAOOxFpXWYRl6zpTKBqtdGOE" "\n"
                "wDzW7WzdyB+dvW/QJ0tESHEpbHdnQJO0dPnjJcbemAjz0CLnCv7Nf5rOgjkCQE3Q" "\n"
                "izIw+/JptmvoOQyx7ixQ2mNCYmpN/Iw63gln0MHaQ5WCPUEmdYWWu3mqmbn7Deaq" "\n"
                "j233Pc4TF7b0FmnaXWsCQAVvyLVU3a9Yactb5MXaN+rEYjUW37GSo+Q1lXfm0OwF" "\n"
                "EJB7X66Bavwg4MCfpGykS71OxhTEfDu+y1gylPMCGHY="                     "\n"
                "-----END RSA PRIVATE KEY-----"                                    "\n"
              , -1), nullptr, nullptr, nullptr)
            )) ctx = nullptr;
          } else {
            if (access(crt.data(), R_OK) == -1)
              Print::logWar("UI", "Unable to read SSL .crt file at " + crt);
            if (access(key.data(), R_OK) == -1)
              Print::logWar("UI", "Unable to read SSL .key file at " + key);
            if (!SSL_CTX_use_certificate_chain_file(ctx, crt.data())
              or !SSL_CTX_use_RSAPrivateKey_file(ctx, key.data(), SSL_FILETYPE_PEM)
            ) {
              ctx = nullptr;
              Print::logWar("UI", "Unable to encrypt web clients, will fallback to plain HTTP");
            }
          }
        }
        return ctx;
      };
      static const string cleanAddress(string addr) {
        if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
        if (addr.length() < 7) addr.clear();
        return addr.empty() ? "unknown" : addr;
      };
  };

  class Events {
    private:
      uS::Timer *timer  = nullptr;
      uS::Async *loop   = nullptr;
      unsigned int tick  = 0,
                   ticks = 300;
      vector<function<const bool(const unsigned int&)>> timeFn;
      vector<function<const bool()>> waitFn;
      vector<function<void()>> slowFn;
    public:
      void timer_ticks_factor(const unsigned int &factor) {
        ticks = 300 * (factor ?: 1);
      };
      void timer_1s(const function<const bool(const unsigned int&)> &fn) {
        timeFn.push_back(fn);
      };
      void wait_for(const function<const bool()> &fn) {
        waitFn.push_back(fn);
      };
      void deferred(const function<void()> &fn) {
        slowFn.push_back(fn);
        loop->send();
      };
    protected:
      void start(uS::Loop *const poll) {
        timer = new uS::Timer(poll);
        timer->setData(this);
        timer->start([](uS::Timer *timer) {
          ((Events*)timer->getData())->timer_1s();
        }, 0, 1e+3);
        loop = new uS::Async(poll);
        loop->setData(this);
        loop->start([](uS::Async *const loop) {
          ((Events*)loop->getData())->deferred();
        });
      };
      function<void()> stop() {
        return [&]() {
          timer->stop();
          deferred();
        };
      };
    private:
      void deferred() {
        for (const auto &it : slowFn) it();
        slowFn.clear();
        bool waiting = false;
        for (const auto &it : waitFn) waiting |= it();
        if (waiting) loop->send();
      };
      void timer_1s() {
        bool waiting = false;
        for (const auto &it : timeFn) waiting |= it(tick);
        if (waiting) loop->send();
        if (++tick >= ticks) tick = 0;
      };
  };

  class mAbout {
    public:
      virtual const mMatter about() const = 0;
  };
  class mBlob: virtual public mAbout {
    public:
      virtual const json blob() const = 0;
  };

  class mFromDb: public mBlob {
    public:
      function<void()> push = []() {
#ifndef NDEBUG
        WARN("Y U NO catch sqlite push?");
#endif
      };
      virtual       void   pull(const json &j) = 0;
      virtual const string increment() const { return "NULL"; };
      virtual const double limit()     const { return 0; };
      virtual const Clock  lifetime()  const { return 0; };
    protected:
      virtual const string explain()   const = 0;
      virtual       string explainOK() const = 0;
      virtual       string explainKO() const { return ""; };
      void explanation(const bool &empty) const {
        string msg = empty
          ? explainKO()
          : explainOK();
        if (msg.empty()) return;
        size_t token = msg.find("%");
        if (token != string::npos)
          msg.replace(token, 1, explain());
        if (empty)
          Print::logWar("DB", msg);
        else Print::log("DB", msg);
      };
  };

  class Sqlite {
    private:
      sqlite3 *db = nullptr;
      string qpdb = "main";
    private_ref:
      Events &events;
    public:
      Sqlite(Events &e)
        : events(e)
      {};
      void backup(mFromDb *const data) {
        if (!db)
          error("DB", "did you miss databases = true; in your constructor?");
        data->pull(select(data));
        data->push = [this, data]() {
          insert(data);
        };
      };
    protected:
      void open(const string &database, const string &diskdata) {
        if (sqlite3_open(database.data(), &db))
          error("DB", sqlite3_errmsg(db));
        Print::log("DB", "loaded OK from", database);
        if (diskdata.empty()) return;
        exec("ATTACH '" + diskdata + "' AS " + (qpdb = "qpdb") + ";");
        Print::log("DB", "loaded OK from", diskdata);
      };
    private:
      const json select(mFromDb *const data) {
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
        const string table    = schema(data->about());
        const json   blob     = data->blob();
        const double limit    = data->limit();
        const Clock  lifetime = data->lifetime();
        const string incr     = data->increment();
        const string sql      = (
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
        events.deferred([this, sql]() {
          exec(sql);
        });
      };
      const string schema(const mMatter &type) const {
        return (
          type == mMatter::QuotingParameters
            ? qpdb
            : "main"
        ) + "." + (char)type;
      };
      const string create(const string &table) const {
        return "CREATE TABLE IF NOT EXISTS " + table + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);";
      };
      const string truncate(const string &table, const Clock &lifetime) const {
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
  };

  //! \brief Deprecated placeholder to avoid spaghetti codes.
  //! - Walks through minimal runtime steps when wait() is called.
  class Klass {
    protected:
      virtual void load        ()    {};
      virtual void waitData    ()   {};
      virtual void waitWebAdmin(){};
      virtual void waitSysAdmin()  {};
      virtual void waitTime    ()  {};
      virtual void run         ()   {};
    public:
      void wait() {
        load();
        waitData();
        waitWebAdmin();
        waitSysAdmin();
        waitTime();
        run();
      };
  };

  class KryptoNinja: public Klass,
                     public Print,
                     public Ending,
                     public Hotkey,
                     public Option,
                     public Socket,
                     public Events,
                     public Sqlite {
    public:
      Gw *gateway = nullptr;
    public:
      KryptoNinja()
        : Sqlite((Events&)*this)
      {};
      KryptoNinja *const main(int argc, char** argv) {
        {
          Option::main(argc, argv);
          setup();
          curl_global_init(CURL_GLOBAL_ALL);
        } {
          if (windowed())
            wait_for(legit_keylogger());
        } {
          log("CF", "Outbound IP address is",
            wtfismyip = Curl::xfer("https://wtfismyip.com/json", 4L)
                          .value("YourFuckingIPAddress", wtfismyip)
          );
        } {
          if (num("latency")) {
            gateway->latency("HTTP read/write handshake", [&]() {
              handshake({
                {"gateway", gateway->http}
              });
            });
            exit("1 HTTP connection done" + Ansi::r(COLOR_WHITE)
              + " (consider to repeat a few times this check)");
          }
        } {
          gateway->socket = socket = new uWS::Hub(0, true);
          gateway->api    = bind();
          start(socket->getLoop());
          ending([&]() {
            gateway->close();
            gateway->api->close();
            gateway->end(num("dustybot"));
            stop();
          });
          wait_for([&]() {
            return gateway->waitForData();
          });
          timer_1s([&](const unsigned int &tick) {
            if (gateway->countdown and !--gateway->countdown) gateway->connect();
            return gateway->countdown ? false : gateway->askForData(tick);
          });
        } {
          if (databases)
            open(str("database"), str("diskdata"));
        }
        return this;
      };
      void wait(const vector<Klass*> &k = {}) {
        if (k.empty()) Klass::wait();
        else for (Klass *const it : k) it->wait();
        if (gateway->ready()) gateway->run();
      };
      void handshake(const vector<pair<string, string>> &notes = {}) {
        const json reply = gateway->handshake();
        if (!gateway->randId or gateway->symbol.empty())
          error("GW", "Incomplete handshake aborted");
        if (!gateway->minTick or !gateway->minSize)
          error("GW", "Unable to fetch data from " + gateway->exchange
            + " for symbol \"" + gateway->symbol + "\", possible error message: "
            + reply.dump());
        gateway->info(notes);
      };
    private:
      void setup() {
        if (!(gateway = Gw::new_Gw(str("exchange"))))
          error("CF",
            "Unable to configure a valid gateway using --exchange="
              + str("exchange") + " argument"
          );
        if (!str("http").empty()) gateway->http = str("http");
        if (!str("wss").empty())  gateway->ws   = str("wss");
        if (!str("fix").empty())  gateway->fix  = str("fix");
        epitaph = "- exchange: " + (gateway->exchange = str("exchange")) + '\n'
                + "- currency: " + (gateway->base     = str("base"))     + " .. "
                                 + (gateway->quote    = str("quote"))    + '\n';
        gateway->apikey   = str("apikey");
        gateway->secret   = str("secret");
        gateway->user     = str("username");
        gateway->pass     = str("passphrase");
        gateway->maxLevel = num("market-limit");
        gateway->debug    = num("debug-secret");
        gateway->version  = num("free-version");
        gateway->printer  = [&](const string &prefix, const string &reason, const string &highlight) {
          if (reason.find("Error") != string::npos)
            Print::logWar(prefix, reason);
          else Print::log(prefix, reason, highlight);
        };
      };
  };
}

#endif
