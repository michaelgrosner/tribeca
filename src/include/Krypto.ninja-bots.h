#ifndef K_BOTS_H_
#define K_BOTS_H_

namespace K {
  string endingMsg, tracelog = to_string(Tstamp);

  vector<function<void()>> happyEndingFn, endingFn = { []() {
    clog << tracelog;
  } };

  class Ending {
    public:
      Ending() {
        signal(SIGINT, quit);
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
        if (code == EXIT_FAILURE)
          this_thread::sleep_for(chrono::seconds(3));
        clog << KOLOR_BGREEN << 'K'
             << KOLOR_RGREEN << " exit code "
             << KOLOR_BGREEN << to_string(code)
             << KOLOR_RGREEN << '.'
             << KOLOR_RRESET << '\n';
        EXIT(code);
      };
      static void quit(const int sig) {
        tracelog = endingMsg + string(endingMsg.empty() ? 0 : 1, '\n');
        halt(EXIT_SUCCESS);
      };
      static void wtf(const int sig) {
        const string rollout = tracelog;
        tracelog = string(KOLOR_RCYAN) + "Errrror: " + strsignal(sig) + ' ';
        const string changelog = mCommand::changelog();
        if (changelog.empty()) {
          tracelog += string("(Three-Headed Monkey found):") + '\n'
            + "- exchange: " + args.exchange     + '\n'
            + "- currency: " + args.currency     + '\n'
            + "- roll-out: " + rollout           + '\n'
            + "- lastbeat: " + to_string(Tstamp) + '\n'
            + "- binbuild: " + string(K_SOURCE)  + ' '
                             + string(K_BUILD)   + '\n'
#ifndef _WIN32
            + "- os-uname: " + mCommand::uname()
            + "- tracelog: " + '\n';
          void *k[69];
          size_t jumps = backtrace(k, 69);
          char **trace = backtrace_symbols(k, jumps);
          for (size_t i = 0; i < jumps; i++)
            tracelog += string(trace[i]) + '\n';
          free(trace)
#endif
          ;
          tracelog += '\n' + string(KOLOR_BRED) + "Yikes!" + string(KOLOR_RRED)
            + '\n' + "please copy and paste the error above into a new github issue (noworry for duplicates)."
            + '\n' + "If you agree, go to https://github.com/ctubio/Krypto-trading-bot/issues/new"
            + '\n' + '\n';
        } else
          tracelog += string("(deprecated K version found).") + '\n'
            + '\n' + string(KOLOR_BYELLOW) + "Hint!" + string(KOLOR_RYELLOW)
            + '\n' + "please upgrade to the latest commit; the encountered error may be already fixed at:"
            + '\n' + changelog
            + '\n' + "If you agree, consider to run \"make latest\" prior further executions."
            + '\n' + '\n';
        halt(EXIT_FAILURE);
      };
  };

  class Rollout: public Ending {
    public:
      Rollout(/* KMxTWEpb9ig */) {
        clog << KOLOR_BGREEN << "K-" << K_SOURCE
             << KOLOR_RGREEN << ' ' << K_BUILD << ' ' << K_STAMP << ".\n";
        const string changelog = mCommand::changelog();
        const int commits = count(changelog.begin(), changelog.end(), '\n');
        clog << KOLOR_BGREEN << K_0_DAY << KOLOR_RGREEN << ' '
             << (commits
                 ? '-' + to_string(commits) + "commit"
                   + string(commits == 1 ? 0 : 1, 's') + '.'
                 : "(0day)"
                )
#ifndef NDEBUG
            << " with DEBUG MODE enabled"
#endif
            << ".\n" << KOLOR_RYELLOW << changelog << KOLOR_RRESET;
      };
  } ____K__B_O_T____;

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
}

#endif
