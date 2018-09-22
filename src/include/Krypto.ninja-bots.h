#ifndef K_BOTS_H_
#define K_BOTS_H_
//! \file
//! \brief Minimal user application framework.

namespace K {
  string epilogue;

  //! \brief     Call all endingFn functions once.
  //! \param[in] reason Allows any (colorful?) string.
  //! \param[in]  code  Must be EXIT_SUCCESS or EXIT_FAILURE.
  void exit(const string &reason = "", const int &code = EXIT_SUCCESS) {
    epilogue = reason;
    raise(code == EXIT_SUCCESS ? SIGQUIT : SIGTERM);
  };

  vector<function<void()>> happyEndingFn, endingFn = { []() {
    clog << epilogue << string(epilogue.empty() ? 0 : 1, '\n');
  } };

  //! \brief Placeholder to avoid spaghetti codes.
  //!
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
        if (!args.headless) waitWebAdmin();
        waitSysAdmin();
        waitTime();
        run();
        endingFn.push_back([&](){
          end();
        });
        if (gw->ready()) gw->run();
      };
  };

  const mClock rollout = Tstamp;

  class Rollout {
    public:
      Rollout(/* KMxTWEpb9ig */) {
        clog << mKolor::b(COLOR_GREEN) << "K-" << K_SOURCE
             << mKolor::r(COLOR_GREEN) << ' ' << K_BUILD << ' ' << K_STAMP << ".\n";
        const string mods = changelog();
        const int commits = count(mods.begin(), mods.end(), '\n');
        clog << mKolor::b(COLOR_GREEN) << K_0_DAY << mKolor::r(COLOR_GREEN) << ' '
             << (commits
                 ? '-' + to_string(commits) + "commit"
                   + string(commits == 1 ? 0 : 1, 's') + '.'
                 : "(0day)"
                )
#ifndef NDEBUG
            << " with DEBUG MODE enabled"
#endif
            << ".\n" << mKolor::r(COLOR_YELLOW) << mods << mKolor::reset();
        mKolor::colorful = 0;
      };
    protected:
      static const string changelog() {
        string mods;
        const json diff = mREST::xfer("https://api.github.com/repos/ctubio/Krypto-trading-bot/compare/" + string(K_0_GIT) + "...HEAD", 4L);
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
        mKolor::colorful = 1;
        clog << mKolor::b(COLOR_GREEN) << 'K'
             << mKolor::r(COLOR_GREEN) << " exit code "
             << mKolor::b(COLOR_GREEN) << code
             << mKolor::r(COLOR_GREEN) << '.'
             << mKolor::reset() << '\n';
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
        halt(EXIT_FAILURE);
      };
      static void wtf(const int sig) {
        epilogue = mKolor::r(COLOR_CYAN) + "Errrror: " + strsignal(sig) + ' ';
        const string mods = changelog();
        if (mods.empty()) {
          epilogue += string("(Three-Headed Monkey found):") + '\n'
            + "- exchange: " + args.exchange                 + '\n'
            + "- currency: " + args.currency                 + '\n'
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
          epilogue += '\n' + mKolor::b(COLOR_RED) + "Yikes!" + mKolor::r(COLOR_RED)
            + '\n' + "please copy and paste the error above into a new github issue (noworry for duplicates)."
            + '\n' + "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
            + '\n' + '\n';
        } else
          epilogue += string("(deprecated K version found).") + '\n'
            + '\n' + mKolor::b(COLOR_YELLOW) + "Hint!" + mKolor::r(COLOR_YELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + mods
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        halt(EXIT_FAILURE);
      };
  } ____K__B_O_T____;
}

#endif
