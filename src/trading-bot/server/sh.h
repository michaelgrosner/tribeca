#ifndef K_SH_H_
#define K_SH_H_

void TradingBot::display() {
  const vector<mOrder> openOrders = engine->orders.working(true);
  const unsigned int previous = Print::margin.bottom;
  Print::margin.bottom = max((int)openOrders.size(), engine->broker.semaphore.paused() ? 0 : 2) + 1;
  int y = getmaxy(stdscr),
      x = getmaxx(stdscr),
      yMaxLog = y - Print::margin.bottom,
      yOrders = yMaxLog;
  if (Print::margin.bottom != previous) {
    if (previous < Print::margin.bottom) wscrl(Print::stdlog, Print::margin.bottom - previous);
    wresize(
      Print::stdlog,
      y - Print::margin.top - Print::margin.bottom,
      x - Print::margin.left - Print::margin.right
    );
    if (previous > Print::margin.bottom) wscrl(Print::stdlog, Print::margin.bottom - previous);
  }
  mvwvline(stdscr, 1, 1, ' ', y-1);
  mvwvline(stdscr, yMaxLog-1, 1, ' ', y-1);
  mvwhline(stdscr, yMaxLog,   1, ' ', x-1);
  for (const mOrder &it : openOrders) {
    mvwhline(stdscr, ++yOrders, 1, ' ', x-1);
    wattron(stdscr, COLOR_PAIR(it.side == Side::Bid ? COLOR_CYAN : COLOR_MAGENTA));
    mvwaddstr(stdscr, yOrders, 1, (((it.side == Side::Bid ? "BID" : "ASK") + (" > "
      + Text::str8(it.quantity))) + ' ' + gw->base + " at price "
      + Text::str8(it.price) + ' ' + gw->quote + " (value "
      + Text::str8(abs(it.price * it.quantity)) + ' ' + gw->quote + ")"
    ).data());
    wattroff(stdscr, COLOR_PAIR(it.side == Side::Bid ? COLOR_CYAN : COLOR_MAGENTA));
  }
  while (++yOrders < y) mvwhline(stdscr, yOrders, 1, ' ', x-1);
  mvwaddch(stdscr, 0, 0, ACS_ULCORNER);
  mvwhline(stdscr, 0, 1, ACS_HLINE, max(80, x));
  mvwhline(stdscr, 1, 14, ' ', max(80, x)-14);
  mvwvline(stdscr, 1, 0, ACS_VLINE, yMaxLog);
  mvwvline(stdscr, yMaxLog, 0, ACS_LTEE, y);
  mvwaddch(stdscr, y, 0, ACS_BTEE);
  mvwaddch(stdscr, 0, 12, ACS_RTEE);
  wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
  string title1 = "   " + K.option.str("exchange");
  string title2 = " " + (K.option.num("port")
    ? "UI at " + Text::strL(client->protocol) + "://" + client->wtfismyip + ":" + K.option.str("port")
    : "headless"
  )  + ' ';
  wattron(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, 13, title1.data());
  wattroff(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, 13+title1.length(), title2.data());
  mvwaddch(stdscr, 0, 14, 'K' | A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
  mvwaddch(stdscr, 0, 13+title1.length()+title2.length(), ACS_LTEE);
  mvwaddch(stdscr, 0, x-26, ACS_RTEE);
  mvwaddstr(stdscr, 0, x-25, (string(" [   ]: ") + (engine->broker.semaphore.paused() ? "Start" : "Stop?") + ", [ ]: Quit!").data());
  mvwaddch(stdscr, 0, x-9, 'q' | A_BOLD);
  wattron(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, x-23, "ESC");
  wattroff(stdscr, A_BOLD);
  mvwaddch(stdscr, 0, 7, ACS_TTEE);
  mvwaddch(stdscr, 1, 7, ACS_LLCORNER);
  mvwhline(stdscr, 1, 8, ACS_HLINE, 4);
  mvwaddch(stdscr, 1, 12, ACS_RTEE);
  wattron(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  const string baseValue  = Text::str8(engine->wallet.base.value),
               quoteValue = Text::str8(engine->wallet.quote.value);
  wattron(stdscr, A_BOLD);
  waddstr(stdscr, (" " + baseValue + ' ').data());
  wattroff(stdscr, A_BOLD);
  waddstr(stdscr, gw->base.data());
  wattroff(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
  waddstr(stdscr, " or ");
  wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
  wattron(stdscr, COLOR_PAIR(COLOR_CYAN));
  wattron(stdscr, A_BOLD);
  waddstr(stdscr, quoteValue.data());
  wattroff(stdscr, A_BOLD);
  waddstr(stdscr, (" " + gw->quote + ' ').data());
  wattroff(stdscr, COLOR_PAIR(COLOR_CYAN));
  size_t xLenValue = 14+baseValue.length()+quoteValue.length()+gw->base.length()+gw->quote.length()+7,
         xMaxValue = max(xLenValue+1, 18+title1.length()+title2.length());
  mvwaddch(stdscr, 0, xMaxValue, ACS_TTEE);
  mvwaddch(stdscr, 1, xMaxValue, ACS_LRCORNER);
  mvwhline(stdscr, 1, xLenValue, ACS_HLINE, xMaxValue - xLenValue);
  mvwaddch(stdscr, 1, xLenValue, ACS_LTEE);
  const int yPos = max(1, (y / 2) - 6),
            baseAmount  = round(engine->wallet.base.amount  * 10 / engine->wallet.base.value),
            baseHeld    = round(engine->wallet.base.held    * 10 / engine->wallet.base.value),
            quoteAmount = round(engine->wallet.quote.amount * 10 / engine->wallet.quote.value),
            quoteHeld   = round(engine->wallet.quote.held   * 10 / engine->wallet.quote.value);
  mvwvline(stdscr, yPos+1, x-3, ' ', 10);
  mvwvline(stdscr, yPos+1, x-4, ' ', 10);
  wattron(stdscr, COLOR_PAIR(COLOR_CYAN));
  mvwvline(stdscr, yPos+11-quoteAmount-quoteHeld, x-4, ACS_VLINE, quoteHeld);
  wattron(stdscr, A_BOLD);
  mvwvline(stdscr, yPos+11-quoteAmount, x-4, ' ' | A_REVERSE, quoteAmount);
  wattroff(stdscr, A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_CYAN));
  wattron(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  mvwvline(stdscr, yPos+11-baseAmount-baseHeld, x-3, ACS_VLINE, baseHeld);
  wattron(stdscr, A_BOLD);
  mvwvline(stdscr, yPos+11-baseAmount, x-3, ' ' | A_REVERSE, baseAmount);
  wattroff(stdscr, A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  mvwaddch(stdscr, yPos, x-2, ACS_URCORNER);
  mvwaddch(stdscr, yPos+11, x-2, ACS_LRCORNER);
  mvwaddch(stdscr, yPos, x-5, ACS_ULCORNER);
  mvwaddch(stdscr, yPos+11, x-5, ACS_LLCORNER);
  mvwhline(stdscr, yPos, x-4, ACS_HLINE, 2);
  mvwhline(stdscr, yPos+11, x-4, ACS_HLINE, 2);
  mvwaddch(stdscr, yMaxLog, 0, ACS_LTEE);
  mvwhline(stdscr, yMaxLog, 1, ACS_HLINE, 3);
  mvwaddch(stdscr, yMaxLog, 4, ACS_RTEE);
  mvwaddstr(stdscr, yMaxLog, 5, "< (");
  if (engine->broker.semaphore.offline()) {
    wattron(stdscr, COLOR_PAIR(COLOR_RED));
    wattron(stdscr, A_BOLD);
    waddstr(stdscr, "DISCONNECTED");
    wattroff(stdscr, A_BOLD);
    wattroff(stdscr, COLOR_PAIR(COLOR_RED));
    waddch(stdscr, ')');
  } else {
    if (engine->broker.semaphore.paused()) {
      wattron(stdscr, COLOR_PAIR(COLOR_YELLOW));
      wattron(stdscr, A_BLINK);
      waddstr(stdscr, "press START to trade");
      wattroff(stdscr, A_BLINK);
      wattroff(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddch(stdscr, ')');
    } else {
      wattron(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddstr(stdscr, to_string(openOrders.size()).data());
      wattroff(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddstr(stdscr, ") Open Orders");
    }
    waddstr(stdscr, " while");
    wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
    waddstr(stdscr, (" 1 " + gw->base + " = ").data());
    wattron(stdscr, A_BOLD);
    waddstr(stdscr, Text::str8(engine->levels.fairValue).data());
    wattroff(stdscr, A_BOLD);
    waddstr(stdscr, (" " + gw->quote).data());
    wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
    waddch(stdscr, engine->broker.semaphore.paused() ? ' ' : ':');
  }
  mvwaddch(stdscr, y-1, 0, ACS_LLCORNER);
  mvwaddstr(stdscr, 1, 2, string("|/-\\").substr(engine->monitor.orders_60s % 4, 1).data());
};

#endif
