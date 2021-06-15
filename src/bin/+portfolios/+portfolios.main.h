class Portfolios: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    Portfolios()
      : engine(*this)
    {
      display = {terminal, {3, 3, 23, 3}};
      events  = {
        [&](const Connectivity &rawdata) { engine.read(rawdata); },
        [&](const Wallet       &rawdata) { engine.read(rawdata); }
      };
    };
  private:
    static void terminal();
} K;

void Portfolios::terminal() {
  const int x = getmaxx(stdscr),
            y = getmaxy(stdscr);
  int yAssets = y - K.padding_bottom(fmin(23, y));
  mvwhline(stdscr, yAssets, 1, ' ', x-1);
  mvwaddstr(stdscr, yAssets++, 1, string(K.engine.broker.ready() ? "online" : "offline").data());
  for (const auto &it : K.engine.wallet.assets) {
    if (yAssets >= y - 1) break;
    mvwhline(stdscr, yAssets, 1, ' ', x-1);
    wattron(stdscr, COLOR_PAIR(it.second.total ? COLOR_GREEN : COLOR_YELLOW));
    mvwaddstr(stdscr, yAssets++, 1, (
      it.second.currency
      + " " + to_string(it.second.amount)
      + " "  + to_string(it.second.held)
      + " "  + to_string(it.second.value)
      + " "  + to_string(it.second.total)
    ).data());
    wattroff(stdscr, COLOR_PAIR(it.second.total ? COLOR_GREEN : COLOR_YELLOW));
  }
};
