extern const char _file_html_index,     _file_css_base,      _file_ico_favicon,
                  _file_gzip_bomb,      _file_css_light,
                  _file_js_client,      _file_css_dark;

extern const  int _file_html_index_len, _file_css_base_len,  _file_ico_favicon_len,
                  _file_gzip_bomb_len,  _file_css_light_len,
                  _file_js_client_len,  _file_css_dark_len;

class Portfolios: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    Portfolios()
      : engine(*this)
    {
      display   = {terminal, {3, 3, 23, 3}};
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata); },
        [&](const Wallet       &rawdata) { engine.read(rawdata); }
      };
      documents = {
        {"",                                  {&_file_gzip_bomb,   _file_gzip_bomb_len  }},
        {"/",                                 {&_file_html_index,  _file_html_index_len }},
        {"/js/client.min.js",                 {&_file_js_client,   _file_js_client_len  }},
        {"/css/bootstrap.min.css",            {&_file_css_base,    _file_css_base_len   }},
        {"/css/bootstrap-theme-dark.min.css", {&_file_css_dark,    _file_css_dark_len   }},
        {"/css/bootstrap-theme.min.css",      {&_file_css_light,   _file_css_light_len  }},
        {"/favicon.ico",                      {&_file_ico_favicon, _file_ico_favicon_len}}
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
