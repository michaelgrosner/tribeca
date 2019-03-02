#ifndef K_GLUE_H_
#define K_GLUE_H_
//! \file
//! \brief Abstract data bindings.

namespace ₿ {
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

  class Backup: public Blob {
    public:
      class Glue {
        public:
          mutable vector<Backup*> tables;
      };
      using Report = pair<bool, string>;
      function<void()> push
#ifndef NDEBUG
        = []() { WARN("Y U NO catch sqlite push?"); }
#endif
      ;
    public:
      Backup(const Glue &glue)
      {
        glue.tables.push_back(this);
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
      StructBackup(const Glue &glue)
        : Backup(glue)
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
      VectorBackup(const Glue &glue)
        : Backup(glue)
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

  class Readable: public Blob {
    public:
      class Glue {
        public:
          mutable vector<Readable*> readable;
      };
      function<void()> read
#ifndef NDEBUG
      = []() { WARN("Y U NO catch read?"); }
#endif
      ;
    public:
      Readable(const Glue &glue)
      {
        glue.readable.push_back(this);
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
      Broadcast(const Glue &glue)
        : Readable(glue)
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
      class Glue {
        public:
          mutable vector<Clickable*> clickable;
      };
    public:
      Clickable(const Glue &glue)
      {
        glue.clickable.push_back(this);
      };
      virtual void click(const json&) = 0;
  };

  class CatchClicks {
    public:
      class Glue {
        public:
          virtual void clicked(const Clickable*, const function<void(const json&)>&) const = 0;
      };
    public:
      CatchClicks(const Glue &glue, const vector<pair<const Clickable*, variant<
        const function<void()>,
        const function<void(const json&)>
      >>> &clicked)
      {
        for (const auto &it : clicked)
          glue.clicked(
            it.first,
            holds_alternative<const function<void()>>(it.second)
              ? [it](const json &j) { get<const function<void()>>(it.second)(); }
              : get<const function<void(const json&)>>(it.second)
          );
      };
  };

  class CatchHotkey {
    public:
      class Glue {
        public:
          virtual void hotkey(const char&, function<void()>) const = 0;
      };
    public:
      CatchHotkey(const Glue &glue, const vector<pair<const char, const function<void()>>> &hotkey)
      {
        for (const auto &it : hotkey)
          glue.hotkey(it.first, it.second);
      };
  };
}

#endif
