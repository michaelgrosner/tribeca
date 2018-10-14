#ifndef K_BOTS_H_
#define K_BOTS_H_
//! \file
//! \brief Minimal user application framework.

namespace K {
  string epilogue;

  vector<function<void()>> happyEndingFn, endingFn = { []() {
    clog << epilogue << string(epilogue.empty() ? 0 : 1, '\n');
  } };

  //! \brief     Call all endingFn once and print a last log msg.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void exit(const string &reason = "", const bool &reboot = false) {
    epilogue = reason + string((reason.empty() or reason.back() == '.') ? 0 : 1, '.');
    raise(reboot ? SIGTERM : SIGQUIT);
  };

  //! \brief Placeholder to avoid spaghetti codes.
  //! - Walks through minimal runtime steps when wait() is called.
  //! - Adds end() into endingFn to be called on exit().
  //! - Connects to gateway when ready.
  class Klass {
    protected:
      virtual void load        ()    {};
      virtual void waitData    ()   {};
      virtual void waitTime    ()  {};
      virtual void waitWebAdmin(){};
      virtual void waitSysAdmin()  {};
      virtual void run         ()   {};
      virtual void end         ()   {};
    public:
      void wait() {
        load();
        waitData();
        waitWebAdmin();
        waitSysAdmin();
        waitTime();
        run();
        endingFn.push_back([&](){
          end();
        });
        if (gw->ready()) gw->run();
      };
  };

  class Ansi {
    public:
      static int colorful;
      static const string reset() {
          return colorful ? "\033[0m" : "";
      };
      static const string b(const int &color) {
          return colorful ? "\033[1;3" + to_string(color) + 'm' : "";
      };
      static const string r(const int &color) {
          return colorful ? "\033[0;3" + to_string(color) + 'm' : "";
      };
  };

  int Ansi::colorful = 1;

  const char *mREST::inet = nullptr;

  //! \brief     Call all endingFn once and print a last error log msg.
  //! \param[in] prefix Allows any string, if possible with a length of 2.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in] reboot Allows a reboot only because https://tldp.org/LDP/Bash-Beginners-Guide/html/sect_09_03.html.
  void error(const string &prefix, const string &reason, const bool &reboot = false) {
    if (reboot) this_thread::sleep_for(chrono::seconds(3));
    exit(prefix + Ansi::r(COLOR_RED) + " Errrror: " + Ansi::b(COLOR_RED) + reason, reboot);
  };

  struct Argument {
   const string  name;
   const string  defined_value;
   const char   *default_value;
   const string  help;
  };
  class Arguments {
    private:
      unordered_map<string, string> optstr;
      unordered_map<string, int>    optint;
      unordered_map<string, double> optdec;
    public:
      virtual void tidy_values(
        unordered_map<string, string> &str,
        unordered_map<string, int>    &num,
        unordered_map<string, double> &dec
      ) {};
      virtual const vector<Argument> custom_long_options() const {
        return {};
      };
      const string str(const string &name) const {
        return optstr.find(name) != optstr.end()
          ? optstr.at(name)
          : (optint.find(name) != optint.end()
              ? to_string(num(name))
              : str8(dec(name))
          );
      };
      const int num(const string &name) const {
        return optint.at(name);
      };
      const double dec(const string &name) const {
        return optdec.at(name);
      };
      void main(int argc, char** argv) {
        vector<Argument> long_options = {
          {"help",         "h",      0,        "show this help and quit"},
          {"version",      "v",      0,        "show current build version and quit"},
          {"autobot",      "1",      0,        "automatically start trading on boot"},
          {"dustybot",     "1",      0,        "do not automatically cancel all orders on exit"},
          {"interface",    "IP",     "",       "set IP to bind as outgoing network interface,"
                                               "\n" "default IP is the system default network interface"},
          {"exchange",     "NAME",   "NULL",   "set exchange NAME for trading, mandatory one of:"
                                               "\n" "'COINBASE', 'BITFINEX',  'BITFINEX_MARGIN',"
                                               "\n" "'HITBTC', 'OKCOIN', 'OKEX', 'KORBIT', 'POLONIEX' or 'NULL'"},
          {"currency",     "PAIR",   "NULL",   "set currency PAIR for trading, use format"
                                               "\n" "with '/' separator, like 'BTC/EUR'"},
          {"apikey",       "WORD",   "NULL",   "set (never share!) WORD as api key for trading, mandatory"},
          {"secret",       "WORD",   "NULL",   "set (never share!) WORD as api secret for trading, mandatory"},
          {"passphrase",   "WORD",   "NULL",   "set (never share!) WORD as api passphrase for trading,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"username",     "WORD",   "NULL",   "set (never share!) WORD as api username for trading,"
                                               "\n" "mandatory but may be 'NULL'"},
          {"http",         "URL",    "NULL",   "set URL of api HTTP/S endpoint for trading, mandatory"},
          {"wss",          "URL",    "NULL",   "set URL of api SECURE WS endpoint for trading, mandatory"},
          {"market-limit", "NUMBER", "321",    "set NUMBER of maximum price levels for the orderbook,"
                                               "\n" "default NUMBER is '321' and the minimum is '15'."
                                               "\n" "locked bots smells like '--market-limit=3' spirit"}
        };
        for (const Argument &it : custom_long_options()) long_options.push_back(it);
        for (const Argument &it : (vector<Argument>){
          {"debug-secret", "1",      0,        "print (never share!) secret inputs and outputs"},
          {"debug",        "1",      0,        "print detailed output about all the (previous) things!"},
          {"colors",       "1",      0,        "print highlighted output"},
          {"title",        "WORD",   K_SOURCE, "set WORD to allow admins to identify different bots"},
          {"free-version", "1",      0,        "work with all market levels and enable the slow XMR miner"}
        }) long_options.push_back(it);
        int index = 1714;
        vector<option> longopts = { {0, 0, 0, 0} };
        for (const Argument &it : long_options) {
          if     (!it.default_value)             optint[it.name] =    !!it.default_value;
          else if (it.defined_value == "NUMBER") optint[it.name] = stoi(it.default_value);
          else if (it.defined_value == "AMOUNT") optdec[it.name] = stod(it.default_value);
          else                                   optstr[it.name] =      it.default_value;
          longopts.insert(longopts.end()-1, {
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
          switch (k = getopt_long(argc, argv, "hv", (option*)&longopts[0], &index)) {
            case -1 :
            case  0 : break;
            case 'h': help(long_options);
            case '?':
            case 'v': EXIT(EXIT_SUCCESS);
            default : {
              const string name(longopts.at(index).name);
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
        Ansi::colorful = num("colors");
        if (!str("interface").empty())
          mREST::inet = str("interface").data();
      };
    private:
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
      void tidy() {
        if (optstr["currency"].find("/") == string::npos or optstr["currency"].length() < 3)
          error("CF", "Invalid --currency value; must be in the format of BASE/QUOTE, like BTC/EUR");
        if (optstr["exchange"].empty())
          error("CF", "Invalid --exchange value; the config file may have errors (there are extra spaces or double defined variables?)");
        optstr["exchange"] = strU(optstr["exchange"]);
        optstr["currency"] = strU(optstr["currency"]);
        optstr["base"]  = optstr["currency"].substr(0, optstr["currency"].find("/"));
        optstr["quote"] = optstr["currency"].substr(1+ optstr["currency"].find("/"));
        optint["market-limit"] = max(15, optint["market-limit"]);
        if (optint["debug"])
          optint["debug-secret"] = 1;
#ifndef _WIN32
        if (optint["debug-secret"])
#endif
          optint["naked"] = 1;
        tidy_values(
          optstr,
          optint,
          optdec
        );
      };
      void gateway() {
        if (!(gw = Gw::new_Gw(str("exchange"))))
          error("CF",
            "Unable to configure a valid gateway using --exchange "
              + str("exchange") + " argument"
          );
        gw->exchange = str("exchange");
        gw->base     = str("base");
        gw->quote    = str("quote");
        gw->apikey   = str("apikey");
        gw->secret   = str("secret");
        gw->user     = str("username");
        gw->pass     = str("passphrase");
        gw->http     = str("http");
        gw->ws       = str("wss");
        gw->autobot  = num("autobot");
        gw->dustybot = num("dustybot");
        gw->maxLevel = num("market-limit");
        gw->debug    = num("debug-secret");
        gw->version  = num("free-version");
      };
  } *args = nullptr;

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
        const json diff = mREST::xfer("https://api.github.com/repos/ctubio/Krypto-trading-bot"
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
        signal(SIGINT, quit);
        signal(SIGQUIT, die);
        signal(SIGTERM, err);
        signal(SIGABRT, wtf);
        signal(SIGSEGV, wtf);
#ifndef _WIN32
        signal(SIGUSR1, wtf);
#endif
      };
    private:
      static void halt(const int &code) {
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
      static void quit(const int sig) {
        clog << '\n';
        die(sig);
      };
      static void die(const int sig) {
        if (epilogue.empty())
          epilogue = "Excellent decision! "
                   + mREST::xfer("https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]", 4L)
                       .value("/value/joke"_json_pointer, "let's plant a tree instead..");
        halt(EXIT_SUCCESS);
      };
      static void err(const int sig) {
        if (epilogue.empty()) epilogue = "Unknown error, no joke.";
        halt(EXIT_FAILURE);
      };
      static void wtf(const int sig) {
        epilogue = Ansi::r(COLOR_CYAN) + "Errrror: " + strsignal(sig) + ' ';
        const string mods = changelog();
        if (mods.empty()) {
          epilogue += string("(Three-Headed Monkey found):") + '\n'
            + "- exchange: " + args->str("exchange")         + '\n'
            + "- currency: " + args->str("currency")         + '\n'
            + "- lastbeat: " + to_string(Tstamp - rollout)   + '\n'
            + "- binbuild: " + string(K_SOURCE)              + ' '
                             + string(K_BUILD)               + '\n'
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
  } ____K__B_O_T____;
}

#endif
