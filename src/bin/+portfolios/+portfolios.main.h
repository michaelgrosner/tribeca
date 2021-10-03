extern const char _file_html_index,     _file_css_base,      _file_ico_favicon,
                  _file_gzip_bomb,      _file_css_light,     _file_woff2_font,
                  _file_js_client,      _file_css_dark;

extern const  int _file_html_index_len, _file_css_base_len,  _file_ico_favicon_len,
                  _file_gzip_bomb_len,  _file_css_light_len, _file_woff2_font_len,
                  _file_js_client_len,  _file_css_dark_len;

class Portfolios: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    Portfolios()
      : engine(*this)
    {
      display   = { terminal };
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata);  },
        [&](const Ticker       &rawdata) { engine.read(rawdata);  },
        [&](const Wallet       &rawdata) { engine.read(rawdata);  },
        [&](const unsigned int &tick)    { engine.timer_1s(tick); }
      };
      documents = {
        {"",                                  {&_file_gzip_bomb,   _file_gzip_bomb_len  }},
        {"/",                                 {&_file_html_index,  _file_html_index_len }},
        {"/js/client.min.js",                 {&_file_js_client,   _file_js_client_len  }},
        {"/css/bootstrap.min.css",            {&_file_css_base,    _file_css_base_len   }},
        {"/css/bootstrap-theme-dark.min.css", {&_file_css_dark,    _file_css_dark_len   }},
        {"/css/bootstrap-theme.min.css",      {&_file_css_light,   _file_css_light_len  }},
        {"/font/beacons.woff2",               {&_file_woff2_font,  _file_woff2_font_len }},
        {"/favicon.ico",                      {&_file_ico_favicon, _file_ico_favicon_len}}
      };
    };
  private:
    static string terminal();
} K;

string Portfolios::terminal() {
  const string quit = "┤ [" + ANSI_HIGH_WHITE
                    + "ESC" + ANSI_PUKE_WHITE + "]: Quit!"
                    + ", [" + ANSI_HIGH_WHITE
                    +  "q"  + ANSI_PUKE_WHITE + "]: Quit!";
  const string title = K.arg<string>("exchange")
                     + ANSI_PUKE_GREEN
                     + ' ' + (K.arg<int>("headless")
                       ? "headless"
                       : "UI at " + K.location()
                     ) + ' ';
  const string top = "┌───────┐ K │ "
                   + ANSI_HIGH_GREEN + title
                   + ANSI_PUKE_WHITE + "├";
  string top_line;
  for (
    unsigned int i = fmax(0,
      K.display.width
      - 1
      - top.length()
      - quit.length()
      + ANSI_SYMBOL_SIZE(12)
      + ANSI_COLORS_SIZE(7)
    );
    i --> 0;
    top_line += "─"
  );
  const string online = string(K.engine.broker.ready() ? "on" : "off")
                      + "line (" + ANSI_HIGH_YELLOW
                      + K.engine.portfolios.settings.currency
                      + ANSI_PUKE_WHITE + ") ├";
  string online_line;
  for (
    unsigned int i = fmax(0,
      title.length()
      - online.length()
      + ANSI_SYMBOL_SIZE(1)
      + ANSI_COLORS_SIZE(1)
    );
    i --> 0;
    online_line += "─"
  );
  unsigned int rows = 0;
  string data;
  for (const auto &it : K.engine.portfolios.portfolio) {
    if (!it.second.wallet.total) continue;
    data += ANSI_PUKE_WHITE + "├──"
          + (it.second.wallet.total ? ANSI_PUKE_GREEN : ANSI_PUKE_YELLOW)
          + ((json)it.second.wallet).dump()
          + ANSI_END_LINE;
    if (++rows == 7) break;
  }
  return ANSI_PUKE_WHITE
    + top + top_line + quit
    + ANSI_END_LINE
    + "│  " + K.spin() + "  └───┤ " + online + online_line + "┘"
    + ANSI_END_LINE
    + K.logs(rows + 3, "│ ")
    + data;
};
