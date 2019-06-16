#ifndef K_BOTS_H_
#define K_BOTS_H_
//! \file
//! \brief Minimal user application framework.

namespace ₿ {
  string epilogue, epitaph;

  //! \brief     Call all endingFn once and print a last log msg.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void exit(const string &reason = "", const bool &reboot = false) {
    epilogue = reason + string((reason.empty() or reason.back() == '.') ? 0 : 1, '.');
    raise(reboot ? SIGTERM : SIGQUIT);
  };

  function<void(CURL*)> Curl::global_setopt = [](CURL *curl) {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
  };

  class Ansi {
    public:
      static int colorful;
      static const string reset() {
          return paint(0, -1);
      };
      static const string r(const int &color) {
          return paint(0, color);
      };
      static const string b(const int &color) {
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
      static const string paint(const int &style, const int &color) {
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

  int Ansi::colorful = 1;

  //! \brief     Call all endingFn once and print a last error log msg.
  //! \param[in] prefix Allows any string, if possible with a length of 2.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void error(const string &prefix, const string &reason, const bool &reboot = false) {
    if (reboot) this_thread::sleep_for(chrono::seconds(3));
    exit(prefix + Ansi::r(COLOR_RED) + " Errrror: " + Ansi::b(COLOR_RED) + reason, reboot);
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
        if (!display) return Ansi::b(COLOR_GREEN) + datetime
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
#ifdef NDEBUG
        version();
#else
        static once_flag test_instance;
        call_once(test_instance, version);
#endif
      };
    protected:
      static void version() {
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
      static const string changelog() {
        string mods;
        const json diff =
#ifdef NDEBUG
          Curl::Web::xfer("https://api.github.com/repos/ctubio/Krypto-trading-bot"
            "/compare/" + string(K_0_GIT) + "...HEAD", 4L)
#else
          json::object()
#endif
        ;
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
                   + Curl::Web::xfer("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
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
      bool autobot = false;
      pair<vector<Argument>, function<void(
        unordered_map<string, variant<string, int, double>>&
      )>> arguments;
    private:
      unordered_map<string, variant<string, int, double>> args;
    public:
      template <typename T> const T arg(const string &name) const {
#ifndef NDEBUG
        if (args.find(name) == args.end()) return T();
#endif
        return get<T>(args.at(name));
      };
    protected:
      void main(int argc, char** argv, const bool &databases, const bool &headless) {
        args["autobot"]  = autobot;
        args["headless"] = headless;
        args["naked"]    = !Print::display;
        vector<Argument> long_options = {
          {"help",         "h",      nullptr,  "show this help and quit"},
          {"version",      "v",      nullptr,  "show current build version and quit"},
          {"latency",      "1",      nullptr,  "check current HTTP latency (not from WS) and quit"},
          {"nocache",      "1",      nullptr,  "do not cache handshakes 7 hours at /var/lib/K/cache"}
        };
        if (!arg<int>("autobot")) long_options.push_back(
          {"autobot",      "1",      nullptr,  "automatically start trading on boot"}
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
            case 'h': help(long_options);
            case '?':
            case 'v': EXIT(EXIT_SUCCESS);
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
        Ansi::colorful = arg<int>("colors");
        if (arguments.second) {
          arguments.second(args);
          arguments.second = nullptr;
        }
        if (arg<int>("naked"))
          Print::display = nullptr;
        if (!arg<string>("interface").empty() and !arg<int>("ipv6"))
          Curl::global_setopt = [&](CURL *curl) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
            curl_easy_setopt(curl, CURLOPT_INTERFACE, arg<string>("interface").data());
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
          };
        else if (!arg<string>("interface").empty())
          Curl::global_setopt = [&](CURL *curl) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
            curl_easy_setopt(curl, CURLOPT_INTERFACE, arg<string>("interface").data());
          };
        else if (!arg<int>("ipv6"))
          Curl::global_setopt = [&](CURL *curl) {
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
        args["market-limit"] = max(15, arg<int>("market-limit"));
        if (arg<int>("debug"))
          args["debug-secret"] = 1;
        if (arg<int>("latency"))
          args["nocache"] = 1;
#if !defined(_WIN32) and defined(NDEBUG)
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

  class Events {
    private:
      uS::Timer *timer = nullptr;
      uS::Async *loop  = nullptr;
              unsigned int tick  = 0;
      mutable unsigned int ticks = 300;
      vector<function<const bool(const unsigned int&)>> timeFn;
      mutable vector<function<const bool()>> waitFn;
      mutable vector<function<void()>> slowFn;
    public:
      void timer_ticks_factor(const unsigned int &factor) const {
        ticks = 300 * (factor ?: 1);
      };
      void timer_1s(const function<const bool(const unsigned int&)> &fn) {
        timeFn.push_back(fn);
      };
      void wait_for(const function<const bool()> &fn) const {
        waitFn.push_back(fn);
      };
      void deferred(const function<void()> &fn) const {
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
      void stop() {
        timer->stop();
        loop->close();
        loop = nullptr;
        deferred();
      };
    private:
      void deferred() {
        for (const auto &it : slowFn) it();
        slowFn.clear();
        if (loop) {
          bool waiting = false;
          for (const auto &it : waitFn) waiting |= it();
          if (waiting) loop->send();
        }
      };
      void timer_1s() {
        bool waiting = false;
        for (const auto &it : timeFn) waiting |= it(tick);
        if (waiting) loop->send();
        if (++tick >= ticks) tick = 0;
      };
  };

  class Hotkey {
    public_friend:
      class Catch {
        public:
          Catch(const Hotkey &hotkey, const vector<pair<const char, const function<void()>>> &hotkeys)
          {
            for (const auto &it : hotkeys)
              hotkey.keymap(it.first, it.second);
          };
      };
    private_ref:
      const Events &events;
    public:
      Hotkey(const Events &e)
        : events(e)
      {};
    private:
      future<char> keylogger;
      mutable unordered_map<char, function<void()>> hotFn;
    protected:
      void legit_keylogger() {
        if (hotFn.empty()) return;
        if (keylogger.valid())
          error("SH", string("Unable to launch another \"keylogger\" thread"));
        noecho();
        halfdelay(5);
        keypad(stdscr, true);
        launch_keylogger();
        events.wait_for([&]() {
          return wait_for_keylog();
        });
      };
    private:
      void keymap(const char &ch, function<void()> fn) const {
        if (hotFn.find(ch) != hotFn.end())
          error("SH", string("Too many handlers for \"") + ch + "\" hotkey event");
        hotFn[ch] = fn;
      };
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
      virtual const mMatter about() const = 0;
      const bool persist() const {
        return about() == mMatter::QuotingParameters;
      };
  };

  class Blob: virtual public About {
    public:
      virtual const json blob() const = 0;
  };

  class Sqlite {
    public_friend:
      class Backup: public Blob {
        public:
          using Report = pair<bool, string>;
          function<void()> push
#ifndef NDEBUG
            = [this]() { WARN("Y U NO catch " << (char)about() << " sqlite push?"); }
#endif
          ;
        public:
          Backup(const Sqlite &sqlite)
          {
            sqlite.tables.push_back(this);
          };
          void backup() const {
            if (push) push();
          };
          virtual const Report pull(const json &j) = 0;
          virtual const string increment() const { return "NULL"; };
          virtual const double limit()     const { return 0; };
          virtual const Clock  lifetime()  const { return 0; };
        protected:
          const Report report(const bool &empty) const {
            string msg = empty
              ? explainKO()
              : explainOK();
            const size_t token = msg.find("%");
            if (token != string::npos)
              msg.replace(token, 1, explain());
            return {empty, msg};
          };
        private:
          virtual const string explain()   const = 0;
          virtual       string explainOK() const = 0;
          virtual       string explainKO() const { return ""; };
      };
      template <typename T> class StructBackup: public Backup {
        public:
          StructBackup(const Sqlite &sqlite)
            : Backup(sqlite)
          {};
          const json blob() const override {
            return *(T*)this;
          };
          const Report pull(const json &j) override {
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
          const Report pull(const json &j) override {
            for (const json &it : j)
              rows.push_back(it);
            return report(empty());
          };
          const json blob() const override {
            return back();
          };
        private:
          const string explain() const override {
            return to_string(size());
          };
      };
    protected:
      bool databases = false;
    private:
      sqlite3 *db = nullptr;
      string disk = "main";
      mutable vector<Backup*> tables;
    private_ref:
      const Events &events;
    public:
      Sqlite(const Events &e)
        : events(e)
      {};
    protected:
      void backups(const string &database, const string &diskdata) {
        if (sqlite3_open(database.data(), &db))
          error("DB", sqlite3_errmsg(db));
        Print::log("DB", "loaded OK from", database);
        if (!diskdata.empty()) {
          exec("ATTACH '" + diskdata + "' AS " + (disk = "disk") + ";");
          Print::log("DB", "loaded OK from", diskdata);
        }
        exec("PRAGMA " + disk + ".journal_mode = WAL;"
             "PRAGMA " + disk + ".synchronous = NORMAL;");
        for (auto &it : tables) {
          report(it->pull(select(it)));
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
      void report(const Backup::Report note) const {
        if (note.second.empty()) return;
        if (note.first)
          Print::logWar("DB", note.second);
        else Print::log("DB", note.second);
      };
      const json select(Backup *const data) {
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
        events.deferred([this, sql]() {
          exec(sql);
        });
      };
      const string schema(Backup *const data) const {
        return (
          data->persist()
            ? disk
            : "main"
        ) + "." + (char)data->about();
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
      void exec(const string &sql, json *const result = nullptr) {              // Print::log("DB DEBUG", sql);
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

  class Client: public WebServer {
    public_friend:
      class Readable: public Blob {
        public:
          function<void()> read
#ifndef NDEBUG
          = [this]() { WARN("Y U NO catch " << (char)about() << " read?"); }
#endif
          ;
        public:
          Readable(const Client &client)
          {
            client.readable.push_back(this);
          };
          virtual const json hello() {
            return { blob() };
          };
          virtual const bool realtime() const {
            return true;
          };
      };
      template <typename T> class Broadcast: public Readable {
        public:
          Broadcast(const Client &client)
            : Readable(client)
          {};
          const bool broadcast() {
            if ((read_asap() or read_soon())
              and (read_same_blob() or diff_blob())
            ) {
              if (read) read();
              return true;
            }
            return false;
          };
          const json blob() const override {
            return *(T*)this;
          };
        protected:
          Clock last_Tstamp = 0;
          string last_blob;
          virtual const bool read_same_blob() const {
            return true;
          };
          const bool diff_blob() {
            const string last = last_blob;
            return (last_blob = blob().dump()) != last;
          };
          virtual const bool read_asap() const {
            return true;
          };
          const bool read_soon(const int &delay = 0) {
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
        public_friend:
          class Catch {
            public:
              Catch(const Client &client, const vector<pair<const Clickable*, variant<
                const function<void()>,
                const function<void(const json&)>
              >>> &clicked)
              {
                for (const auto &it : clicked)
                  client.clicked(
                    it.first,
                    holds_alternative<const function<void()>>(it.second)
                      ? [it](const json &j) { get<const function<void()>>(it.second)(); }
                      : get<const function<void(const json&)>>(it.second)
                  );
              };
          };
      };
    public:
      string protocol  = "HTTP";
      string wtfismyip = "localhost";
    protected:
      unordered_map<string, pair<const char*, const int>> documents;
    private:
      SSL_CTX *ctx = nullptr;
      curl_socket_t sockfd = 0;
      vector<Frontend> requests,
                       sockets;
      mutable unsigned int delay = 0;
      mutable vector<Readable*> readable;
      mutable vector<Clickable*> clickable;
      mutable unordered_map<const Clickable*, vector<function<void(const json&)>>> clickFn;
      const pair<char, char> portal = {'=', '-'};
      unordered_map<char, function<const json()>> hello;
      unordered_map<char, function<void(const json&)>> kisses;
      unordered_map<char, string> queue;
    private_ref:
      const Option &option;
    public:
      Client(const Option &o)
        : option(o)
      {};
      void listen() {
        if (!(sockfd = WebServer::listen(
          option.arg<string>("interface"),
          option.arg<int>("port"),
          option.arg<int>("ipv6")
        ))) error("UI", "Unable to listen at port number " + to_string(option.arg<int>("port"))
              + " (may be already in use by another program)");
        if (!option.arg<int>("without-ssl")) {
          for (const auto &it : ssl_context(
            option.arg<string>("ssl-crt"),
            option.arg<string>("ssl-key"),
            ctx
          )) Print::logWar("UI", it);
          protocol += string(ctx ? 1 : 0, 'S');
        }
        Print::log("UI", "ready at", Text::strL(protocol) + "://" + wtfismyip + ":" + to_string(option.arg<int>("port")));
      };
      void clicked(const Clickable *data, const json &j = nullptr) const {
        if (clickFn.find(data) != clickFn.end())
          for (const auto &it : clickFn.at(data)) it(j);
      };
      void client_queue_delay(const unsigned int &d) const {
        delay = d;
      };
      void broadcast(const unsigned int &tick) {
        if (delay and !(tick % delay)) broadcast();
      };
      void welcome() {
        for (auto &it : readable) {
          it->read = [&]() {
            if (sockets.size()) {
              queue[(char)it->about()] = it->blob().dump();
              if (it->realtime() or !delay) broadcast();
            }
          };
          hello[(char)it->about()] = [&]() {
            return it->hello();
          };
        }
        readable.clear();
        for (auto &it : clickable) {
          kisses[(char)it->about()] = [&](const json &butterfly) {
            it->click(butterfly);
          };
        }
        clickable.clear();
      };
      void headless() {
        for (auto &it : readable)
          it->read = nullptr;
        readable.clear();
        clickable.clear();
        documents.clear();
      };
      const int clients() {
        while (accept_requests(sockfd, ctx, requests));
        for (auto it = requests.begin(); it != requests.end();) {
          if (Tstamp > it->time + 21e+3)
            shutdown(it->sockfd, it->ssl);
          else io(it->sockfd, it->ssl, it->in, it->out, false);
          if (it->sockfd
            and it->out.empty()
            and it->in.length() > 5
            and it->in.substr(0, 5) == "GET /"
            and it->in.find("\r\n\r\n") != string::npos
          ) {
            const string path   = it->in.substr(4, it->in.find(" HTTP/1.1") - 4);
            const string addr   = address(it->sockfd);
            const size_t papers = it->in.find("Authorization: Basic ");
            string auth;
            if (papers != string::npos) {
              auth = it->in.substr(papers + 21);
              auth = auth.substr(0, auth.find("\r\n"));
            }
            const size_t key = it->in.find("Sec-WebSocket-Key: ");
            int allowed = 1;
            if (key == string::npos) {
              it->out = httpResponse(path, auth, addr);
              if (it->out.empty())
                shutdown(it->sockfd, it->ssl);
            } else if ((option.arg<string>("B64auth").empty() or auth == option.arg<string>("B64auth"))
              and it->in.find("Upgrade: websocket" "\r\n") != string::npos
              and it->in.find("Connection: Upgrade" "\r\n") != string::npos
              and it->in.find("Sec-WebSocket-Version: 13" "\r\n") != string::npos
              and (allowed = httpUpgrade(allowed, addr))
            ) {
              sockets.push_back({
                it->sockfd,
                it->ssl,
                Tstamp,
                addr,
                "HTTP/1.1 101 Switching Protocols"
                "\r\n" "Connection: Upgrade"
                "\r\n" "Upgrade: websocket"
                "\r\n" "Sec-WebSocket-Version: 13"
                "\r\n" "Sec-WebSocket-Accept: "
                         + Text::B64(Text::SHA1(
                           it->in.substr(key + 19, it->in.substr(key + 19).find("\r\n"))
                             + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                           true
                         )) +
                "\r\n"
                "\r\n"
              });
              it->sockfd = 0;
            } else {
              if (!allowed) httpUpgrade(allowed, addr);
              shutdown(it->sockfd, it->ssl);
            }
            it->in.clear();
          }
          if (!it->sockfd)
            it = requests.erase(it);
          else ++it;
        }
        for (auto it = sockets.begin(); it != sockets.end();) {
          io(it->sockfd, it->ssl, it->in, it->out, true);
          if (it->sockfd and !it->in.empty()) {
            const string msg = unframe(it->sockfd, it->ssl, it->in, it->out);
            if (!msg.empty()) {
              string reply = httpMessage(msg, it->addr);
              if (!reply.empty())
                it->out += frame(reply, reply.substr(0, 2) == "PK" ? 0x02 : 0x01, false);
            }
          }
          if (!it->sockfd) {
            httpUpgrade(-1, it->addr);
            it = sockets.erase(it);
          } else ++it;
        }
        return requests.size()
             + sockets.size();
      };
      void withoutGoodbye() {
        for (auto &it : requests)
          shutdown(it.sockfd, it.ssl);
        int i = 0;
        for (auto &it : sockets) {
          httpUpgrade(--i, it.addr);
          shutdown(it.sockfd, it.ssl);
        }
        shutdown(sockfd);
      };
    private:
      void clicked(const Clickable *data, const function<void(const json&)> &fn) const {
        clickFn[data].push_back(fn);
      };
      void broadcast() {
        if (!sockets.empty() and !queue.empty()) {
          string msgs;
          for (const auto &it : queue)
            msgs += frame(portal.second + (it.first + it.second), 0x01, false);
          for (auto &it : sockets)
            it.out += msgs;
        }
        queue.clear();
      };
      const bool alien(const string &addr) {
        if (addr != "unknown"
          and !option.arg<string>("whitelist").empty()
          and option.arg<string>("whitelist").find(addr) == string::npos
        ) {
          Print::log("UI", "dropping gzip bomb on", addr);
          return true;
        }
        return false;
      };
      const int httpUpgrade(const int &sum, const string &addr) {
        const int tentative = sockets.size() + sum;
        Print::log("UI", to_string(tentative) + " client" + string(tentative == 1 ? 0 : 1, 's')
          + (tentative > 0 ? "" : " remain") + " connected, last connection was from", addr);
        if (tentative > option.arg<int>("client-limit")) {
          Print::log("UI", "--client-limit=" + to_string(option.arg<int>("client-limit")) + " reached by", addr);
          return 0;
        }
        return sum;
      };
      const string httpMessage(string message, const string &addr) {
        if (alien(addr))
          return string(documents.at("").first, documents.at("").second);
        const char matter = message.at(1);
        if (portal.first == message.at(0)) {
          if (hello.find(matter) != hello.end()) {
            const json reply = hello.at(matter)();
            if (!reply.is_null())
              return portal.first + (matter + reply.dump());
          }
        } else if (portal.second == message.at(0) and kisses.find(matter) != kisses.end()) {
          message = message.substr(2);
          json butterfly = json::accept(message)
            ? json::parse(message)
            : json::object();
          for (auto it = butterfly.begin(); it != butterfly.end();)
            if (it.value().is_null()) it = butterfly.erase(it); else ++it;
          kisses.at(matter)(butterfly);
        }
        return string();
      };
      const string httpResponse(string path, const string &auth, const string &addr) {
        if (alien(addr))
          path.clear();
        const bool papersplease = !(
          path.empty() or option.arg<string>("B64auth").empty()
        );
        string content,
               type = "text/html; charset=UTF-8";
        unsigned int code = 200;
        const string leaf = path.substr(path.find_last_of('.') + 1);
        if (papersplease and auth.empty()) {
          Print::log("UI", "authorization attempt from", addr);
          code = 401;
        } else if (papersplease and auth != option.arg<string>("B64auth")) {
          Print::log("UI", "authorization failed from", addr);
          code = 403;
        } else if (leaf != "/" or sockets.size() < option.arg<int>("client-limit")) {
          if (documents.find(path) == documents.end())
            path = path.substr(path.find_last_of("/", path.find_last_of("/") - 1));
          if (documents.find(path) == documents.end())
            path = path.substr(path.find_last_of("/"));
          if (documents.find(path) != documents.end()) {
            content = string(documents.at(path).first,
                             documents.at(path).second);
            if (leaf == "/") Print::log("UI", "authorization success from", addr);
            else if (leaf == "js")  type = "application/javascript; charset=UTF-8";
            else if (leaf == "css") type = "text/css; charset=UTF-8";
            else if (leaf == "ico") type = "image/x-icon";
            else if (leaf == "mp3") type = "audio/mpeg";
          } else {
            if (Random::int64() % 21)
              code = 404, content = "Today, is a beautiful day.";
            else // Humans! go to any random path to check your luck
              code = 418, content = "Today, is your lucky day!";
          }
        } else {
          Print::log("UI", "--client-limit=" + to_string(option.arg<int>("client-limit")) + " reached by", addr);
          content = "Thank you! but our princess is already in this castle!"
                    "<br/>" "Refresh the page anytime to retry.";
        }
        return document(content, code, type);
      };
  };

  //! \brief Placeholder to avoid spaghetti codes.
  //! - Walks through minimal runtime steps when wait() is called.
  class Klass {
    protected:
      virtual void waitData () {};
      virtual void waitAdmin(){};
      virtual void run      () {};
    public:
      void wait() {
        waitData();
        waitAdmin();
        run();
      };
  };

  class KryptoNinja: public Klass,
                     public Print,
                     public Ending,
                     public Option,
                     public Events,
                     public Hotkey,
                     public Sqlite,
                     public Client {
    public:
      Gw *gateway = nullptr;
    private:
      uWS::Hub *hub = nullptr;
    public:
      KryptoNinja()
        : Hotkey((Events&)*this)
        , Sqlite((Events&)*this)
        , Client((Option&)*this)
      {};
      KryptoNinja *const main(int argc, char** argv) {
        {
          curl_global_init(CURL_GLOBAL_ALL);
          Option::main(argc, argv, databases, documents.empty());
          setup();
        } {
          if (windowed()) legit_keylogger();
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
          hub = new uWS::Hub(0, true);
          start(hub->getLoop());
          ending([&]() {
            gateway->end(arg<int>("dustybot"));
            stop();
            curl_global_cleanup();
          });
          wait_for([&]() {
            return gateway->waitForData();
          });
          timer_1s([&](const unsigned int &tick) {
            return gateway->askForData(tick);
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
          if (databases)
            backups(
              arg<string>("database"),
              arg<string>("diskdata")
            );
          else blackhole();
        } {
          if (arg<int>("headless")) headless();
          else {
            listen();
            timer_1s([&](const unsigned int &tick) {
              broadcast(tick);
              return clients();
            });
            wait_for([&]() {
              return clients();
            });
            ending([&]() {
              withoutGoodbye();
            });
            welcome();
          }
        }
        return this;
      };
      void wait(Klass *const k = nullptr) {
        if (k) k->wait();
        else Klass::wait();
        if (gateway->ready())
          hub->run();
      };
      void handshake(const GwExchange::Report &notes = {}) {
        const json reply = gateway->handshake(arg<int>("nocache"));
        if (!gateway->tickPrice or !gateway->tickSize or !gateway->minSize)
          error("GW", "Unable to fetch data from " + gateway->exchange
            + " for symbols " + gateway->base + "/" + gateway->quote
            + ", possible error message: " + reply.dump());
        gateway->report(notes, arg<int>("nocache"));
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
        if (!databases or arg<string>("database") == ":memory:") return 0;
        struct stat st;
        return stat(arg<string>("database").data(), &st) ? 0 : st.st_size;
      };
    private:
      void setup() {
        if (!(gateway = Gw::new_Gw(arg<string>("exchange"))))
          error("CF",
            "Unable to configure a valid gateway using --exchange="
              + arg<string>("exchange") + " argument"
          );
        epitaph = "- exchange: " + (gateway->exchange = arg<string>("exchange")) + '\n'
                + "- currency: " + (gateway->base     = arg<string>("base"))     + " .. "
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
        gateway->apikey    = arg<string>("apikey");
        gateway->secret    = arg<string>("secret");
        gateway->pass      = arg<string>("passphrase");
        gateway->maxLevel  = arg<int>("market-limit");
        gateway->debug     = arg<int>("debug-secret");
        gateway->version   = arg<int>("free-version");
        gateway->printer   = [&](const string &prefix, const string &reason, const string &highlight) {
          if (reason.find("Error") != string::npos)
            Print::logWar(prefix, reason);
          else Print::log(prefix, reason, highlight);
        };
      };
  };
}

#endif
