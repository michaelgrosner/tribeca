#ifndef K_TRADING_BOT_H_
#define K_TRADING_BOT_H_

extern const char _www_html_index,     _www_ico_favicon,     _www_css_base,
                  _www_gzip_bomb,      _www_mp3_audio_0,     _www_css_light,
                  _www_js_client,      _www_mp3_audio_1,     _www_css_dark;

extern const  int _www_html_index_len, _www_ico_favicon_len, _www_css_base_len,
                  _www_gzip_bomb_len,  _www_mp3_audio_0_len, _www_css_light_len,
                  _www_js_client_len,  _www_mp3_audio_1_len, _www_css_dark_len;

class TradingBot: public KryptoNinja {
  public:
    static void terminal();
    TradingBot()
    {
      display   = terminal;
      margin    = {3, 6, 1, 2};
      databases = true;
      arguments = { {
        {"wallet-limit", "AMOUNT", "0",                    "set AMOUNT in base currency to limit the balance,"
                                                           "\n" "otherwise the full available balance can be used"},
        {"client-limit", "NUMBER", "7",                    "set NUMBER of maximum concurrent UI connections"},
        {"headless",     "1",      nullptr,                "do not listen for UI connections,"
                                                           "\n" "all other UI related arguments will be ignored"},
        {"without-ssl",  "1",      nullptr,                "do not use HTTPS for UI connections (use HTTP only)"},
        {"ssl-crt",      "FILE",   "",                     "set FILE to custom SSL .crt file for HTTPS UI connections"
                                                           "\n" "(see www.akadia.com/services/ssh_test_certificate.html)"},
        {"ssl-key",      "FILE",   "",                     "set FILE to custom SSL .key file for HTTPS UI connections"
                                                           "\n" "(the passphrase MUST be removed from the .key file!)"},
        {"whitelist",    "IP",     "",                     "set IP or csv of IPs to allow UI connections,"
                                                           "\n" "alien IPs will get a zip-bomb instead"},
        {"port",         "NUMBER", "3000",                 "set NUMBER of an open port to listen for UI connections"},
        {"user",         "WORD",   "NULL",                 "set allowed WORD as username for UI connections,"
                                                           "\n" "mandatory but may be 'NULL'"},
        {"pass",         "WORD",   "NULL",                 "set allowed WORD as password for UI connections,"
                                                           "\n" "mandatory but may be 'NULL'"},
        {"lifetime",     "NUMBER", "0",                    "set NUMBER of minimum milliseconds to keep orders open,"
                                                           "\n" "otherwise open orders can be replaced anytime required"},
        {"matryoshka",   "URL",    "https://example.com/", "set Matryoshka link URL of the next UI"},
        {"ignore-sun",   "2",      nullptr,                "do not switch UI to light theme on daylight"},
        {"ignore-moon",  "1",      nullptr,                "do not switch UI to dark theme on moonlight"},
        {"autobot",      "1",      nullptr,                "automatically start trading on boot"},
        {"debug-orders", "1",      nullptr,                "print detailed output about exchange messages"},
        {"debug-quotes", "1",      nullptr,                "print detailed output about quoting engine"},
        {"debug-wallet", "1",      nullptr,                "print detailed output about target base position"}
      }, [](
        unordered_map<string, string> &str,
        unordered_map<string, int>    &num,
        unordered_map<string, double> &dec
      ) {
        if (num["debug"])
          num["debug-orders"] =
          num["debug-quotes"] =
          num["debug-wallet"] = 1;
        if (num["ignore-moon"] and num["ignore-sun"])
          num["ignore-moon"] = 0;
        if (num["debug-orders"] or num["debug-quotes"])
          num["naked"] = 1;
        if (num["latency"] or !num["port"] or !num["client-limit"])
          num["headless"] = 1;
        str["B64auth"] = (!num["headless"]
          and str["user"] != "NULL" and !str["user"].empty()
          and str["pass"] != "NULL" and !str["pass"].empty()
        ) ? "Basic " + Text::B64(str["user"] + ':' + str["pass"])
          : "";
      } };
    };
} K;

#include "server/if.h"
#include "server/sh.h"

#endif
