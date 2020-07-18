//! \file
//! \brief Minimal user application framework.

namespace ₿ {
  static string epilogue, epitaph;

  //! \brief     Call all endingFn once and print a last log msg.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  static void exit(const string &reason = "", const bool &reboot = false) {
    epilogue = reason + string((reason.empty() or reason.back() == '.') ? 0 : 1, '.');
    raise(reboot ? SIGTERM : SIGQUIT);
  };

  static int colorful = 1;

  class Ansi {
    public:
      static string reset() {
          return paint(0, -1);
      };
      static string r(const int &color) {
          return paint(0, color);
      };
      static string b(const int &color) {
          return paint(1, color);
      };
      static void default_colors() {
        if (colorful) start_color();
        use_default_colors();
        for (const auto &color : {
          COLOR_BLACK,
          COLOR_RED,
          COLOR_GREEN,
          COLOR_YELLOW,
          COLOR_BLUE,
          COLOR_MAGENTA,
          COLOR_CYAN,
          COLOR_WHITE
        }) init_pair(
          color,
          color,
          color
            ? COLOR_BLACK
            : COLOR_WHITE
        );
      };
    private:
      static string paint(const int &style, const int &color) {
        return colorful
          ? "\033["
            + to_string(style)
            + (color == -1
              ? ""
              : ";3" + to_string(color)
            ) + 'm'
          : "";
      };
  };

  //! \brief     Call all endingFn once and print a last error log msg.
  //! \param[in] prefix Allows any string, if possible with a length of 2.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  static void error(const string &prefix, const string &reason, const bool &reboot = false) {
    if (reboot) this_thread::sleep_for(chrono::seconds(3));
    exit(prefix + Ansi::r(COLOR_RED) + " Errrror: " + Ansi::b(COLOR_RED) + reason, reboot);
  };

  class Rollout {
    public:
      Rollout(/* KMxTWEpb9ig */) {
        static once_flag rollout;
        call_once(rollout, version);
      };
    protected:
      static void version() {
        curl_global_init(CURL_GLOBAL_ALL);
        clog << Ansi::b(COLOR_GREEN) << K_SOURCE
             << Ansi::r(COLOR_GREEN) << " " K_BUILD " " K_STAMP ".\n";
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
      static string changelog() {
        string mods;
        const json diff =
#ifndef NDEBUG
          json::object();
#else
          Curl::Web::xfer("https://api.github.com/repos/ctubio/"
            "Krypto-trading-bot/compare/" K_HEAD "...HEAD", 4L);
#endif
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

  static vector<function<void()>> endingFn;

  class Ending: public Rollout {
    public:
      Ending() {
        signal(SIGINT, [](const int) {
          clog << '\n';
          raise(SIGQUIT);
        });
        signal(SIGQUIT, die);
        signal(SIGTERM, err);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
        signal(SIGUSR1, wtf);
        signal(SIGPIPE, SIG_IGN);
      };
      void ending(const function<void()> &fn) {
        endingFn.push_back(fn);
      };
    private:
      static void halt(const int code) {
        vector<function<void()>> happyEndingFn;
        endingFn.swap(happyEndingFn);
        for (const auto &it : happyEndingFn) it();
        colorful = 1;
        clog << Ansi::b(COLOR_GREEN) << 'K'
             << Ansi::r(COLOR_GREEN) << " exit code "
             << Ansi::b(COLOR_GREEN) << code
             << Ansi::r(COLOR_GREEN) << '.'
             << Ansi::reset() << '\n';
        EXIT(code);
      };
      static void die(const int) {
        if (epilogue.empty())
          epilogue = "Excellent decision! "
                   + Curl::Web::xfer("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
                       .value("/value/joke"_json_pointer, "let's plant a tree instead..");
        halt(
          epilogue.find("Errrror") == string::npos
            ? EXIT_SUCCESS
            : EXIT_FAILURE
        );
      };
      static void err(const int) {
        if (epilogue.empty()) epilogue = "Unknown error, no joke.";
        halt(EXIT_FAILURE);
      };
      static void wtf(const int sig) {
        epilogue = Ansi::r(COLOR_CYAN) + "Errrror: " + strsignal(sig) + ' ';
        const string mods = changelog();
        if (mods.empty()) {
          epilogue += "(Three-Headed Monkey found):\n" + epitaph
             + "- binbuild: " K_SOURCE " " K_BUILD "\n"
               "- lastbeat: " + to_string((float)clock()/CLOCKS_PER_SEC) + '\n'
#ifndef _WIN32
            + "- tracelog: " + '\n';
          void *k[69];
          size_t jumps = backtrace(k, 69);
          char **trace = backtrace_symbols(k, jumps);
          for (;
            jumps --> 0;
            epilogue += "  " + to_string(jumps) + ": " + string(trace[jumps]) + '\n'
          );
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

  class Terminal {
    public:
      struct {
        void (*terminal)() = nullptr;
        mutable struct {
          unsigned int top;
          unsigned int right;
          unsigned int bottom;
          unsigned int left;
        } padding = {ANY_NUM, 0, 0, 0};
      } display;
      WINDOW *stdlog = nullptr;
    public:
      unsigned int padding_bottom(const unsigned int &bottom) const {
        if (bottom != display.padding.bottom) {
          const int diff = bottom - display.padding.bottom;
          display.padding.bottom = bottom;
          if (diff > 0) wscrl(stdlog, diff);
          wresize(
            stdlog,
            getmaxy(stdscr) - display.padding.top - display.padding.bottom,
            getmaxx(stdscr) - display.padding.left - display.padding.right
          );
          if (diff < 0) wscrl(stdlog, diff);
        }
        return display.padding.bottom;
      };
      void repaint() const {
        if (!(stdscr and display.terminal)) return;
        display.terminal();
        wrefresh(stdscr);
        if (stdlog) {
          wmove(stdlog, getmaxy(stdlog) - 1, 0);
          wrefresh(stdlog);
        }
      };
      string stamp() const {
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
        if (!display.terminal)
          return Ansi::b(COLOR_GREEN) + datetime
               + Ansi::r(COLOR_GREEN) + microtime.str()
               + Ansi::b(COLOR_WHITE) + ' ';
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
      void log(const string &prefix, const string &reason, const string &highlight = "") const {
        int color = 0;
        if (reason.find("NG TRADE") != string::npos) {
          if (reason.find("BUY") != string::npos)       color = 1;
          else if (reason.find("SELL") != string::npos) color = -1;
        }
        if (!display.terminal) {
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
      void logWar(const string &prefix, const string &reason) const {
        if (!display.terminal) {
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
    protected:
      bool windowed() {
        if (!display.terminal) return false;
        if (stdlog)
          error("SH", "Unable to print another window");
        if (!initscr())
          error("SH",
            "Unable to initialize ncurses, try to run in your terminal"
              "\"export TERM=xterm\", or use --naked argument"
          );
        Ansi::default_colors();
        if (display.padding.top != ANY_NUM) {
          stdlog = subwin(
            stdscr,
            getmaxy(stdscr) - display.padding.bottom - display.padding.top,
            getmaxx(stdscr) - display.padding.left - display.padding.right,
            display.padding.top,
            display.padding.left
          );
          scrollok(stdlog, true);
          idlok(stdlog, true);
        }
        signal(SIGWINCH, [](const int) {
          endwin();
          refresh();
          clear();
        });
        repaint();
        return true;
      };
  };

  class Option: public Terminal {
    private_friend:
      struct Argument {
       const string  name;
       const string  defined_value;
       const char   *default_value;
       const string  help;
      };
    protected:
      bool autobot  = false;
      bool dustybot = false;
      using MutableUserArguments = unordered_map<string, variant<string, int, double>>;
      pair<vector<Argument>, function<void(MutableUserArguments&)>> arguments;
    private:
      MutableUserArguments args;
    public:
      template <typename T> const T arg(const string &name) const {
#ifndef NDEBUG
        if (args.find(name) == args.end()) return T();
#endif
        return get<T>(args.at(name));
      };
    protected:
      void main(Ending *const K, int argc, char** argv, const bool &databases, const bool &headless) {
        K->ending([&]() {
          if (display.terminal) {
            display = {};
            beep();
            endwin();
          }
          clog << stamp()
               << epilogue
               << string(epilogue.empty() ? 0 : 1, '\n');
        });
        args["autobot"]  = autobot;
        args["dustybot"] = dustybot;
        args["headless"] = headless;
        args["naked"]    = !display.terminal;
        vector<Argument> long_options = {
          {"help",         "h",      nullptr,  "show this help and quit"},
          {"version",      "v",      nullptr,  "show current build version and quit"},
          {"latency",      "1",      nullptr,  "check current HTTP latency (not from WS) and quit"},
          {"nocache",      "1",      nullptr,  "do not cache handshakes 7 hours at /var/lib/K/cache"}
        };
        if (!arg<int>("autobot")) long_options.push_back(
          {"autobot",      "1",      nullptr,  "automatically start trading on boot"}
        );
        if (!arg<int>("dustybot")) long_options.push_back(
          {"dustybot",     "1",      nullptr,  "do not automatically cancel all orders on exit"}
        );
        if (!arg<int>("naked")) long_options.push_back(
          {"naked",        "1",      nullptr,  "do not display CLI, print output to stdout instead"}
        );
        if (databases) long_options.push_back(
          {"database",     "FILE",   "",       "set alternative PATH to database filename,"
                                               "\n" "default PATH is '/var/lib/K/db/K-*.db',"
                                               "\n" "or use ':memory:' (see sqlite.org/inmemorydb.html)"}
        );
        if (!arg<int>("headless")) for (const Argument &it : (vector<Argument>){
          {"headless",     "1",      nullptr,  "do not listen for UI connections,"
                                               "\n" "all other UI related arguments will be ignored"},
          {"without-ssl",  "1",      nullptr,  "do not use HTTPS for UI connections (use HTTP only)"},
          {"whitelist",    "IP",     "",       "set IP or csv of IPs to allow UI connections,"
                                               "\n" "alien IPs will get a zip-bomb instead"},
          {"client-limit", "NUMBER", "7",      "set NUMBER of maximum concurrent UI connections"},
          {"port",         "NUMBER", "3000",   "set NUMBER of an open port to listen for UI connections"},
          {"user",         "WORD",   "NULL",   "set allowed WORD as username for UI connections,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"pass",         "WORD",   "NULL",   "set allowed WORD as password for UI connections,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"ssl-crt",      "FILE",   "",       "set FILE to custom SSL .crt file for HTTPS UI connections"
                                               "\n" "(see www.akadia.com/services/ssh_test_certificate.html)"},
          {"ssl-key",      "FILE",   "",       "set FILE to custom SSL .key file for HTTPS UI connections"
                                               "\n" "(the passphrase MUST be removed from the .key file!)"}
        }) long_options.push_back(it);
        for (const Argument &it : (vector<Argument>){
          {"interface",    "IP",     "",       "set IP to bind as outgoing network interface"},
          {"ipv6",         "1",      nullptr,  "use IPv6 when possible"},
          {"exchange",     "NAME",   "NULL",   "set exchange NAME for trading, mandatory"},
          {"currency",     "PAIR",   "NULL",   "set currency PAIR for trading, use format"
                                               "\n" "with '/' separator, like 'BTC/EUR'"},
          {"apikey",       "WORD",   "NULL",   "set (never share!) WORD as api key for trading, mandatory"},
          {"secret",       "WORD",   "NULL",   "set (never share!) WORD as api secret for trading, mandatory"},
          {"passphrase",   "WORD",   "NULL",   "set (never share!) WORD as api passphrase for trading,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"maker-fee",    "AMOUNT", "0",      "set custom percentage of maker fee, like '0.1'"},
          {"taker-fee",    "AMOUNT", "0",      "set custom percentage of taker fee, like '0.1'"},
          {"min-size",     "AMOUNT", "0",      "set custom minimum order size, like '0.01'"},
          {"leverage",     "AMOUNT", "1",      "set between '0.01' and '100' to enable isolated margin,"
                                               "\n" "or use '0' for cross margin; default AMOUNT is '1'"},
          {"http",         "URL",    "",       "set URL of alernative HTTPS api endpoint for trading"},
          {"wss",          "URL",    "",       "set URL of alernative WSS api endpoint for trading"},
          {"fix",          "URL",    "",       "set URL of alernative FIX api endpoint for trading"},
          {"market-limit", "NUMBER", "321",    "set NUMBER of maximum price levels for the orderbook,"
                                               "\n" "default NUMBER is '321' and the minimum is '10'"}
        }) long_options.push_back(it);
        for (const Argument &it : arguments.first)
          long_options.push_back(it);
        arguments.first.clear();
        for (const Argument &it : (vector<Argument>){
          {"debug-secret", "1",      nullptr,  "print (never share!) secret inputs and outputs"},
          {"debug",        "1",      nullptr,  "print detailed output about all the (previous) things!"},
          {"colors",       "1",      nullptr,  "print highlighted output"},
          {"title",        "WORD",   K_SOURCE, "set WORD to allow admins to identify different bots"},
          {"free-version", "1",      nullptr,  "slowdown market levels 7 seconds"}
        }) long_options.push_back(it);
        int index = ANY_NUM;
        vector<option> opt_long = { {nullptr, 0, nullptr, 0} };
        for (const Argument &it : long_options) {
          if     (!it.default_value)             args[it.name] = 0;
          else if (it.defined_value == "NUMBER") args[it.name] = stoi(it.default_value);
          else if (it.defined_value == "AMOUNT") args[it.name] = stod(it.default_value);
          else                                   args[it.name] =      it.default_value;
          opt_long.insert(opt_long.end()-1, {
            it.name.data(),
            it.default_value
              ? required_argument
              : no_argument,
            it.default_value or it.defined_value.at(0) > '>'
              ? nullptr
              : get_if<int>(&args.at(it.name)),
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
            case 'h': help(long_options); [[fallthrough]];
            case '?':
            case 'v': EXIT(EXIT_SUCCESS);                                       //-V796
            default : {
              const string name(opt_long.at(index).name);
              if      (holds_alternative<int>(args[name]))    args[name] =   stoi(optarg);
              else if (holds_alternative<double>(args[name])) args[name] =   stod(optarg);
              else if (holds_alternative<string>(args[name])) args[name] = string(optarg);
            }
          }
        if (optind < argc) {
          string argerr = "Unhandled argument option(s):";
          while(optind < argc) argerr += string(" ") + argv[optind++];
          error("CF", argerr);
        }
        tidy();
        colorful = arg<int>("colors");
        if (arguments.second) {
          arguments.second(args);
          arguments.second = nullptr;
        }
        if (arg<int>("naked"))
          display = {};
        if (!arg<string>("interface").empty() and !arg<int>("ipv6"))
          curl_global_setopt = [this](CURL *curl) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
            curl_easy_setopt(curl, CURLOPT_INTERFACE, arg<string>("interface").data());
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
          };
        else if (!arg<string>("interface").empty())
          curl_global_setopt = [this](CURL *curl) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
            curl_easy_setopt(curl, CURLOPT_INTERFACE, arg<string>("interface").data());
          };
        else if (!arg<int>("ipv6"))
          curl_global_setopt = [](CURL *curl) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
          };
      };
    private:
      void tidy() {
        if (arg<string>("currency").find("/") == string::npos or arg<string>("currency").length() < 3)
          error("CF", "Invalid --currency value; must be in the format of BASE/QUOTE, like BTC/EUR");
        if (arg<string>("exchange").empty())
          error("CF", "Invalid --exchange value; the config file may have errors (there are extra spaces or double defined variables?)");
        args["exchange"] = Text::strU(arg<string>("exchange"));
        args["currency"] = Text::strU(arg<string>("currency"));
        args["base"]  = Text::strU(arg<string>("currency").substr(0, arg<string>("currency").find("/")));
        args["quote"] = Text::strU(arg<string>("currency").substr(1+ arg<string>("currency").find("/")));
        args["market-limit"] = max(10, arg<int>("market-limit"));
        args["leverage"] = fmax(0, fmin(100, arg<double>("leverage")));
        if (arg<int>("debug"))
          args["debug-secret"] = 1;
        if (arg<int>("latency"))
          args["nocache"] = 1;
#if !defined _WIN32 and defined NDEBUG
        if (arg<int>("latency") or arg<int>("debug-secret"))
#endif
          args["naked"] = 1;
        if (args.find("database") != args.end()) {
          args["diskdata"] = "";
          if (arg<string>("database").empty() or arg<string>("database") == ":memory:")
            (arg<string>("database") == ":memory:"
              ? args["diskdata"]
              : args["database"]
            ) = ("/var/lib/K/db/" K_SOURCE)
              + ('.' + arg<string>("exchange"))
              +  '.' + arg<string>("base")
              +  '.' + arg<string>("quote")
              +  '.' + "db";
        }
        if (!arg<int>("headless")) {
          if (arg<int>("latency") or !arg<int>("port") or !arg<int>("client-limit"))
            args["headless"] = 1;
          args["B64auth"] = (!arg<int>("headless")
            and arg<string>("user") != "NULL" and !arg<string>("user").empty()
            and arg<string>("pass") != "NULL" and !arg<string>("pass").empty()
          ) ? Text::B64(arg<string>("user") + ':' + arg<string>("pass"))
            : "";
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
          << Ansi::b(COLOR_WHITE) << stamp.at(((++y%4)*3)+x) << "Usage:" << Ansi::b(COLOR_YELLOW) << " " << K_SOURCE " [arguments]" << '\n';
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

  class Hotkey {
    public_friend:
      class Keymap {
        public:
          Keymap(const Hotkey &hotkey, const vector<pair<const char, const function<void()>>> &hotkeys)
          {
            for (const auto &it : hotkeys)
              hotkey.keymap(it.first, it.second);
          };
      };
    public:
      ~Hotkey()
      {
        stop = true;
      };
    private:
      bool stop = false;
      Loop::Async::Event<char> keylogger;
      mutable unordered_map<char, function<void()>> maps;
    protected:
      void wait_for_keylog(Loop *const loop) {
        if (maps.empty()) return;
        if (keylogger.write)
          error("SH", string("Unable to launch another \"keylogger\" thread"));
        noecho();
        halfdelay(5);
        keypad(stdscr, true);
        keylogger.write = [this](const char &ch) { keylog(ch); };
        keylogger.wait_for(loop, [this]() { return sync_keylogger(); });
        keylogger.ask_for();
      };
    private:
      void keymap(const char &ch, function<void()> fn) const {
        if (maps.find(ch) != maps.end())
          error("SH", string("Too many handlers for \"") + ch + "\" hotkey event");
        maps[ch] = fn;
      };
      void keylog(const char &ch) {
        if (maps.find(ch) != maps.end())
          maps.at(ch)();
        keylogger.ask_for();
      };
      vector<char> sync_keylogger() {
        int ch = ERR;
        while (ch == ERR and !stop)
          ch = getch();
        return {
          ch == ERR
           ? '\r'
           : (char)ch
        };
      };
  };

  class About {
    public:
      enum class mMatter: char {
        FairValue            = 'a',                                                       Connectivity       = 'd',
        MarketData           = 'e', QuotingParameters    = 'f',
        OrderStatusReports   = 'i', ProductAdvertisement = 'j', ApplicationState   = 'k', EWMAStats          = 'l',
        STDEVStats           = 'm', Position             = 'n', Profit             = 'o', SubmitNewOrder     = 'p',
        CancelOrder          = 'q', MarketTrade          = 'r', Trades             = 's',
        QuoteStatus          = 'u', TargetBasePosition   = 'v', TradeSafetyValue   = 'w', CancelAllOrders    = 'x',
        CleanAllClosedTrades = 'y', CleanAllTrades       = 'z', CleanTrade         = 'A',
                                    MarketChart          = 'D', Notepad            = 'E',
                                    MarketDataLongTerm   = 'H'
      };
    public:
      virtual mMatter about() const = 0;
      bool persist() const {
        return about() == mMatter::QuotingParameters;
      };
  };

  class Blob: virtual public About {
    public:
      virtual json blob() const = 0;
  };

  class Sqlite {
    public_friend:
      class Backup: public Blob {
        public:
          using Report = pair<bool, string>;
          function<void()> push;
        public:
          Backup(const Sqlite &sqlite)
          {
            sqlite.tables.push_back(this);
          };
          void backup() const {
            if (push) push();
          };
          virtual Report pull(const json &j) = 0;
          virtual string increment() const { return "NULL"; };
          virtual double limit()     const { return 0; };
          virtual Clock  lifetime()  const { return 0; };
        protected:
          Report report(const bool &empty) const {
            string msg = empty
              ? explainKO()
              : explainOK();
            const size_t token = msg.find("%");
            if (token != string::npos)
              msg.replace(token, 1, explain());
            return {empty, msg};
          };
        private:
          virtual string explain()   const = 0;
          virtual string explainOK() const = 0;
          virtual string explainKO() const { return ""; };
      };
      template <typename T> class StructBackup: public Backup {
        public:
          StructBackup(const Sqlite &sqlite)
            : Backup(sqlite)
          {};
          json blob() const override {
            return *(T*)this;
          };
          Report pull(const json &j) override {
            from_json(j.empty() ? blob() : j.at(0), *(T*)this);
            return report(j.empty());
          };
        private:
          string explainOK() const override {
            return "loaded last % OK";
          };
      };
      template <typename T> class VectorBackup: public Backup {
        public:
          VectorBackup(const Sqlite &sqlite)
            : Backup(sqlite)
          {};
          vector<T> rows;
          using reference              = typename vector<T>::reference;
          using const_reference        = typename vector<T>::const_reference;
          using iterator               = typename vector<T>::iterator;
          using const_iterator         = typename vector<T>::const_iterator;
          using reverse_iterator       = typename vector<T>::reverse_iterator;
          using const_reverse_iterator = typename vector<T>::const_reverse_iterator;
          iterator                 begin()       noexcept { return rows.begin();   };
          const_iterator           begin() const noexcept { return rows.begin();   };
          const_iterator          cbegin() const noexcept { return rows.cbegin();  };
          iterator                   end()       noexcept { return rows.end();     };
          const_iterator             end() const noexcept { return rows.end();     };
          reverse_iterator        rbegin()       noexcept { return rows.rbegin();  };
          const_reverse_iterator crbegin() const noexcept { return rows.crbegin(); };
          reverse_iterator          rend()       noexcept { return rows.rend();    };
          bool                     empty() const noexcept { return rows.empty();   };
          size_t                    size() const noexcept { return rows.size();    };
          reference                front()                { return rows.front();   };
          const_reference          front() const          { return rows.front();   };
          reference                 back()                { return rows.back();    };
          const_reference           back() const          { return rows.back();    };
          reference                   at(size_t n)        { return rows.at(n);     };
          const_reference             at(size_t n) const  { return rows.at(n);     };
          virtual void erase() {
            if (size() > limit())
              rows.erase(begin(), end() - limit());
          };
          virtual void push_back(const T &row) {
            rows.push_back(row);
            backup();
            erase();
          };
          Report pull(const json &j) override {
            for (const json &it : j)
              rows.push_back(it);
            return report(empty());
          };
          json blob() const override {
            return back();
          };
        private:
          string explain() const override {
            return to_string(size());
          };
      };
    protected:
      bool databases = false;
    private:
      sqlite3 *db = nullptr;
      string disk = "main";
      mutable vector<Backup*> tables;
    protected:
      void backups(const Option *const K) {
        if (sqlite3_open(K->arg<string>("database").data(), &db))
          error("DB", sqlite3_errmsg(db));
        K->log("DB", "loaded OK from", K->arg<string>("database"));
        if (!K->arg<string>("diskdata").empty()) {
          exec("ATTACH '" + K->arg<string>("diskdata") + "' AS " + (disk = "disk") + ";");
            K->log("DB", "loaded OK from", K->arg<string>("diskdata"));
        }
        exec("PRAGMA " + disk + ".journal_mode = WAL;"
             "PRAGMA " + disk + ".synchronous = NORMAL;");
        for (auto &it : tables) {
          const Backup::Report note = it->pull(select(it));
          if (!note.second.empty()) {
            if (note.first)
              K->logWar("DB", note.second);
            else K->log("DB", note.second);
          }
          it->push = [this, it]() {
            insert(it);
          };
        }
        tables.clear();
      };
      void blackhole() {
        for (auto &it : tables)
          it->push = nullptr;
        tables.clear();
      };
    private:
      json select(Backup *const data) {
        const string table = schema(data);
        json result = json::array();
        exec(
          create(table)
          + truncate(table, data->lifetime())
          + "SELECT json FROM " + table + " ORDER BY time ASC;",
          &result
        );
        return result;
      };
      void insert(Backup *const data) {
        const string table    = schema(data);
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
        exec(sql);
      };
      string schema(Backup *const data) const {
        return (
          data->persist()
            ? disk
            : "main"
        ) + "." + (char)data->about();
      };
      string create(const string &table) const {
        return "CREATE TABLE IF NOT EXISTS " + table + "("
          + "id    INTEGER   PRIMARY KEY AUTOINCREMENT                                           NOT NULL,"
          + "json  BLOB                                                                          NOT NULL,"
          + "time  TIMESTAMP DEFAULT (CAST((julianday('now') - 2440587.5)*86400000 AS INTEGER))  NOT NULL);";
      };
      string truncate(const string &table, const Clock &lifetime) const {
        return lifetime
          ? "DELETE FROM " + table + " WHERE time < " + to_string(Tstamp - lifetime) + ";"
          : "";
      };
      void exec(const string &sql, json *const result = nullptr) {
        char* zErrMsg = nullptr;
        sqlite3_exec(db, sql.data(), result ? write : nullptr, (void*)result, &zErrMsg);
        if (zErrMsg) error("DB", "SQLite error: " + (zErrMsg + (" at " + sql)));
        sqlite3_free(zErrMsg);
      };
      static int write(void *result, int argc, char **argv, char**) {
        for (int i = 0; i < argc; ++i)
          ((json*)result)->push_back(json::parse(argv[i]));
        return 0;
      };
  };

  class Client {
    public_friend:
      class Readable: public Blob {
        public:
          function<void()> read;
        public:
          Readable(const Client &client)
          {
            client.readable.push_back(this);
          };
          virtual json hello() {
            return { blob() };
          };
          virtual bool realtime() const {
            return true;
          };
      };
      template <typename T> class Broadcast: public Readable {
        public:
          Broadcast(const Client &client)
            : Readable(client)
          {};
          bool broadcast() {
            if ((read_asap() or read_soon())
              and (read_same_blob() or diff_blob())
            ) {
              if (read) read();
              return true;
            }
            return false;
          };
          json blob() const override {
            return *(T*)this;
          };
        protected:
          Clock last_Tstamp = 0;
          string last_blob;
          virtual bool read_same_blob() const {
            return true;
          };
          bool diff_blob() {
            const string last = last_blob;
            return (last_blob = blob().dump()) != last;
          };
          virtual bool read_asap() const {
            return true;
          };
          bool read_soon(const int &delay = 0) {
            const Clock now = Tstamp;
            if (last_Tstamp + max(369, delay) > now)
              return false;
            last_Tstamp = now;
            return true;
          };
      };
      class Clickable: virtual public About {
        public:
          Clickable(const Client &client)
          {
            client.clickable.push_back(this);
          };
          virtual void click(const json&) = 0;
      };
      class Clicked {
        public:
          Clicked(const Client &client, const vector<pair<const Clickable*, variant<
            const function<void()>,
            const function<void(const json&)>
          >>> &clicked)
          {
            for (const auto &it : clicked)
              client.clicked(
                it.first,
                holds_alternative<const function<void()>>(it.second)
                  ? [it](const json&) { get<const function<void()>>(it.second)(); }
                  : get<const function<void(const json&)>>(it.second)
              );
          };
      };
    protected:
      string wtfismyip = "localhost";
      unordered_map<string, pair<const char*, const int>> documents;
    private:
      string protocol = "HTTP";
      WebServer::Backend server;
      const Option *option = nullptr;
      mutable unsigned int delay = 0;
      mutable vector<Readable*> readable;
      mutable vector<Clickable*> clickable;
      mutable unordered_map<const Clickable*, vector<function<void(const json&)>>> clickFn;
      const pair<char, char> portal = {'=', '-'};
      unordered_map<char, function<json()>> hello;
      unordered_map<char, function<void(const json&)>> kisses;
      unordered_map<char, string> queue;
    public:
      void listen(const Option *const K, const curl_socket_t &loopfd) {
        option = K;
        if (!server.listen(
          loopfd,
          K->arg<string>("interface"),
          K->arg<int>("port"),
          K->arg<int>("ipv6"),
          {
            K->arg<string>("B64auth"),
            response,
            upgrade,
            message
          }
        )) error("UI", "Unable to listen at port number " + to_string(K->arg<int>("port"))
             + " (may be already in use by another program)");
        if (!K->arg<int>("without-ssl"))
          for (const auto &it : server.ssl_context(
            K->arg<string>("ssl-crt"),
            K->arg<string>("ssl-key")
          )) K->logWar("UI", it);
        protocol  = server.protocol();
        K->log("UI", "ready at", location());
      };
      string location() const {
        return option
          ? Text::strL(protocol) + "://" + wtfismyip + ":" + to_string(option->arg<int>("port"))
          : "loading..";
      };
      void clicked(const Clickable *data, const json &j = nullptr) const {
        if (clickFn.find(data) != clickFn.end())
          for (const auto &it : clickFn.at(data)) it(j);
      };
      void client_queue_delay(const unsigned int &d) const {
        delay = d;
      };
      void broadcast(const unsigned int &tick) {
        if (delay and !(tick % delay))
          broadcast();
        server.timeouts();
      };
      void welcome() {
        for (auto &it : readable) {
          it->read = [this, it]() {
            if (server.idle()) return;
            queue[(char)it->about()] = it->blob().dump();
            if (it->realtime() or !delay) broadcast();
          };
          hello[(char)it->about()] = [it]() {
            return it->hello();
          };
        }
        readable.clear();
        for (auto &it : clickable)
          kisses[(char)it->about()] = [it](const json &butterfly) {
            it->click(butterfly);
          };
        clickable.clear();
      };
      void headless() {
        for (auto &it : readable)
          it->read = nullptr;
        readable.clear();
        clickable.clear();
        documents.clear();
      };
      void without_goodbye() {
        server.purge();
      };
    private:
      void clicked(const Clickable *data, const function<void(const json&)> &fn) const {
        clickFn[data].push_back(fn);
      };
      void broadcast() {
        if (queue.empty()) return;
        if (!server.idle())
          server.broadcast(portal.second, queue);
        queue.clear();
      };
      bool alien(const string &addr) {
        if (addr != "unknown"
          and !option->arg<string>("whitelist").empty()
          and option->arg<string>("whitelist").find(addr) == string::npos
        ) {
          option->log("UI", "dropping gzip bomb on", addr);
          return true;
        }
        return false;
      };
      WebServer::Response response = [&](string path, const string &auth, const string &addr) {
        if (alien(addr))
          path.clear();
        const bool papersplease = !(path.empty() or option->arg<string>("B64auth").empty());
        string content,
               type = "text/html; charset=UTF-8";
        unsigned int code = 200;
        const string leaf = path.substr(path.find_last_of('.') + 1);
        if (papersplease and auth.empty()) {
          option->log("UI", "authorization attempt from", addr);
          code = 401;
        } else if (papersplease and auth != option->arg<string>("B64auth")) {
          option->log("UI", "authorization failed from", addr);
          code = 403;
        } else if (leaf != "/" or server.clients() < option->arg<int>("client-limit")) {
          if (documents.find(path) == documents.end())
            path = path.substr(path.find_last_of("/", path.find_last_of("/") - 1));
          if (documents.find(path) == documents.end())
            path = path.substr(path.find_last_of("/"));
          if (documents.find(path) != documents.end()) {
            content = string(documents.at(path).first,
                             documents.at(path).second);
            if (leaf == "/") option->log("UI", "authorization success from", addr);
            else if (leaf == "js")  type = "application/javascript; charset=UTF-8";
            else if (leaf == "css") type = "text/css; charset=UTF-8";
            else if (leaf == "ico") type = "image/x-icon";
            else if (leaf == "mp3") type = "audio/mpeg";
          } else {
            if (Random::int64() % 21)
              code = 404, content = "Today, is a beautiful day.";
            else // Humans! go to any random path to check your luck.
              code = 418, content = "Today, is your lucky day!";
          }
        } else {
          option->log("UI", "--client-limit=" + to_string(option->arg<int>("client-limit"))
            + " reached by", addr);
          content = "Thank you! but our princess is already in this castle!"
                    "<br/>" "Refresh the page anytime to retry.";
        }
        return server.document(content, code, type);
      };
      WebServer::Upgrade upgrade = [&](const int &sum, const string &addr) {
        const int tentative = server.clients() + sum;
        option->log("UI", to_string(tentative) + " client" + string(tentative == 1 ? 0 : 1, 's')
          + (sum > 0 ? "" : " remain") + " connected, last connection was from", addr);
        if (tentative > option->arg<int>("client-limit")) {
          option->log("UI", "--client-limit=" + to_string(option->arg<int>("client-limit"))
            + " reached by", addr);
          return 0;
        }
        return sum;
      };
      WebServer::Message message = [&](string msg, const string &addr) {
        if (alien(addr))
          return string(documents.at("").first, documents.at("").second);
        const char matter = msg.at(1);
        if (portal.first == msg.at(0)) {
          if (hello.find(matter) != hello.end()) {
            const json reply = hello.at(matter)();
            if (!reply.is_null())
              return portal.first + (matter + reply.dump());
          }
        } else if (portal.second == msg.at(0) and kisses.find(matter) != kisses.end()) {
          msg = msg.substr(2);
          json butterfly = json::accept(msg)
            ? json::parse(msg)
            : json::object();
          for (auto it = butterfly.begin(); it != butterfly.end();)
            if (it.value().is_null()) it = butterfly.erase(it); else ++it;
          kisses.at(matter)(butterfly);
        }
        return string();
      };
  };

  class KryptoNinja: public Events,
                     public Ending,
                     public Option,
                     public Hotkey,
                     public Sqlite,
                     public Client {
    public:
      Gw *gateway = nullptr;
    protected:
      vector<variant<TimeEvent, Gw::DataEvent>> events;
    public:
      KryptoNinja *main(int argc, char** argv) {
        {
          Option::main(this, argc, argv, databases, documents.empty());
          setup();
        } {
          if (windowed())
            wait_for_keylog(this);
        } {
          log("CF", "Outbound IP address is",
            wtfismyip = Curl::Web::xfer("https://wtfismyip.com/json", 4L)
                          .value("YourFuckingIPAddress", wtfismyip)
          );
        } {
          if (arg<int>("latency")) {
            gateway->latency("HTTP read/write handshake", [&]() {
              handshake({
                {"gateway", gateway->http}
              });
            });
            exit("1 HTTP connection done" + Ansi::r(COLOR_WHITE)
              + " (consider to repeat a few times this check)");
          }
        } {
          for (const auto &it : events)
            if (holds_alternative<Gw::DataEvent>(it))
              gateway->data(get<Gw::DataEvent>(it));
        } {
          gateway->wait_for_data(this);
          timer_1s([&](const unsigned int &tick) {
            gateway->ask_for_data(tick);
          });
          ending([&]() {
            if (!dustybot)
              gateway->purge(arg<int>("dustybot"));
            gateway->end();
            end();
          });
          handshake({
            {"gateway", gateway->http      },
            {"gateway", gateway->ws        },
            {"gateway", gateway->fix       },
            {"autoBot", arg<int>("autobot")
                          ? "yes"
                          : "no"           }
          });
        } {
          for (const auto &it : events)
            if (holds_alternative<TimeEvent>(it))
              timer_1s(get<TimeEvent>(it));
          events.clear();
        } {
          if (databases)
            backups(this);
          else blackhole();
        } {
          if (arg<int>("headless")) headless();
          else {
            listen(this, poll());
            timer_1s([&](const unsigned int &tick) {
              broadcast(tick);
            });
            ending([&]() {
              without_goodbye();
            });
            welcome();
          }
        } {
          ending([&]() {
            curl_global_cleanup();
            if (!arg<int>("free-version"))
              gateway->disclaimer();
          });
        }
        return this;
      };
      void wait() {
        walk();
      };
      unsigned int dbSize() const {
        if (!databases or arg<string>("database") == ":memory:") return 0;
        struct stat st;
        return stat(arg<string>("database").data(), &st) ? 0 : st.st_size;
      };
      unsigned int memSize() const {
#ifdef _WIN32
        return 0;
#else
        struct rusage ru;
        return getrusage(RUSAGE_SELF, &ru) ? 0 : ru.ru_maxrss * 1e+3;
#endif
      };
    private:
      void handshake(const GwExchange::Report &notes = {}) {
        const json reply = gateway->handshake(arg<int>("nocache"));
        if (!gateway->tickPrice or !gateway->tickSize or !gateway->minSize)
          error("GW", "Unable to fetch data from " + gateway->exchange
            + " for symbols " + gateway->base + "/" + gateway->quote
            + ", possible error message: " + reply.dump());
        gateway->report(notes, arg<int>("nocache"));
      };
      void setup() {
        if (!(gateway = Gw::new_Gw(arg<string>("exchange"))))
          error("CF",
            "Unable to configure a valid gateway using --exchange="
              + arg<string>("exchange") + " argument"
          );
        epitaph = "- exchange: " + (gateway->exchange = arg<string>("exchange")) + '\n'
                + "- currency: " + (gateway->base     = arg<string>("base"))     + "/"
                                 + (gateway->quote    = arg<string>("quote"))    + '\n';
        if (!gateway->http.empty() and !arg<string>("http").empty())
          gateway->http    = arg<string>("http");
        if (!gateway->ws.empty() and !arg<string>("wss").empty())
          gateway->ws      = arg<string>("wss");
        if (!gateway->fix.empty() and !arg<string>("fix").empty())
          gateway->fix     = arg<string>("fix");
        if (arg<double>("taker-fee"))
          gateway->takeFee = arg<double>("taker-fee") / 1e+2;
        if (arg<double>("maker-fee"))
          gateway->makeFee = arg<double>("maker-fee") / 1e+2;
        if (arg<double>("min-size"))
          gateway->minSize = arg<double>("min-size");
        gateway->leverage  = arg<double>("leverage");
        gateway->apikey    = arg<string>("apikey");
        gateway->secret    = arg<string>("secret");
        gateway->pass      = arg<string>("passphrase");
        gateway->maxLevel  = arg<int>("market-limit");
        gateway->debug     = arg<int>("debug-secret");
        gateway->loopfd    = poll();
        gateway->printer   = [&](const string &prefix, const string &reason, const string &highlight) {
          if (reason.find("Error") != string::npos)
            logWar(prefix, reason);
          else log(prefix, reason, highlight);
        };
        gateway->adminAgreement = (Connectivity)arg<int>("autobot");
      };
  };
}
