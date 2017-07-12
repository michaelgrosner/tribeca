import Models = require("../share/models");

export class Publisher {
  private _lastMarketData: number = new Date().getTime();

  public publish = (topic: string, msg: any, monitor?: boolean) => {
    if (topic === Models.Topics.MarketData) {
      if (this._lastMarketData+369>new Date().getTime()) return;
      msg = this.compressMarketDataInc(msg);
    } else if (topic === Models.Topics.OrderStatusReports)
      msg = this.compressOSRInc(msg);
    else if (topic === Models.Topics.Position)
      msg = this.compressPositionInc(msg);
    if (monitor) this.delay(topic, msg);
    else this._socket.up(topic, msg);
  };

  public registerSnapshot = (topic, snapshot) => {
    this._socket.on(Models.Prefixes.SNAPSHOT + topic, (topic) => {
      let snap: any[];
      if (topic === Models.Topics.MarketData)
        snap = this.compressSnapshot(snapshot(), this.compressMarketDataInc);
      else if (topic === Models.Topics.OrderStatusReports)
        snap = this.compressSnapshot(snapshot(), this.compressOSRInc);
      else if (topic === Models.Topics.Position)
        snap = this.compressSnapshot(snapshot(), this.compressPositionInc);
      else snap = snapshot();
      return snap;
    });
  }

  private compressSnapshot = (data: any[], compressIncremental:(data: any) => any): any[] => {
    let ret: any[] = [];
    data.forEach(x => ret.push(compressIncremental(x)));
    return ret;
  };

  private compressMarketDataInc = (data: any): any => {
    let ret: any = new Models.Timestamped([[],[]], data.time);
    data.bids.forEach(bid => ret.data[0].push(bid.price, bid.size));
    data.asks.forEach(ask => ret.data[1].push(ask.price, ask.size));
    this._lastMarketData = new Date().getTime();
    return ret;
  };

  private compressOSRInc = (data: any): any => {
    return <any>new Models.Timestamped(
      (data.orderStatus == Models.OrderStatus.Cancelled
      || data.orderStatus == Models.OrderStatus.Complete)
    ? [
      data.orderId,
      data.orderStatus,
      data.side,
      data.price,
      data.quantity
    ] : [
      data.orderId,
      data.orderStatus,
      data.side,
      data.exchange,
      data.price,
      data.quantity,
      data.type,
      data.timeInForce,
      data.computationalLatency,
      data.leavesQuantity,
      data.isPong,
      data.pair.quote
    ], data.time);
  };

  private compressPositionInc = (data: any): any => {
    return <any>new Models.Timestamped([
      data.baseAmount,
      data.quoteAmount,
      data.baseHeldAmount,
      data.quoteHeldAmount,
      data.value,
      data.quoteValue,
      data.profitBase,
      data.profitQuote,
      data.pair.base,
      data.pair.quote
    ], data.time);
  };

  constructor(
    private _dbSize,
    private _socket,
    private _evOn,
    private _delayUI,
    public registerReceiver
  ) {
    this.setTick();
    this._evOn('QuotingParameters', (qp) => {
      this._delayUI = qp.delayUI;
      this.setTick();
    });

    this.registerSnapshot(Models.Topics.ApplicationState, () => [this._app_state]);

    this.registerSnapshot(Models.Topics.Notepad, () => [this._notepad]);

    this.registerSnapshot(Models.Topics.ToggleConfigs, () => [this._toggleConfigs]);

    this.registerReceiver(Models.Topics.Notepad, (notepad: string) => {
      this._notepad = notepad;
    });

    this.registerReceiver(Models.Topics.ToggleConfigs, (toggleConfigs: boolean) => {
      this._toggleConfigs = toggleConfigs;
    });
  }

  private _delayed: any[] = [];
  private _app_state: Models.ApplicationState;
  private _notepad: string;
  private _toggleConfigs: boolean = true;
  private _newOrderMinute: number = 0;
  private _tick: number = 0;
  private _interval = null;

  private onTick = () => {
    this._tick = 0;
    this._app_state = new Models.ApplicationState(
      process.memoryUsage().rss,
      (new Date()).getHours(),
      this._newOrderMinute / 2,
      this._dbSize()
    );
    this._newOrderMinute = 0;
    this.publish(Models.Topics.ApplicationState, this._app_state);
  };

  private onDelay = () => {
    this._tick += this._delayUI;
    if (this._tick>=6e1) this.onTick();
    let orders: any[] = this._delayed.filter(x => x[0]===Models.Topics.OrderStatusReports);
    this._delayed = this._delayed.filter(x => x[0]!==Models.Topics.OrderStatusReports);
    if (orders.length) this._delayed.push([Models.Topics.OrderStatusReports, {data: orders.map(x => x[1])}]);
    this._delayed.forEach(x => this._socket.up(x[0], x[1]));
    this._delayed = orders.filter(x => x[1].data[1]===Models.OrderStatus.Working);
  };

  private delay = (topic: string, msg: any) => {
    const isOSR: boolean = topic === Models.Topics.OrderStatusReports;
    if (isOSR && msg.data[1] === Models.OrderStatus.New) return ++this._newOrderMinute;
    if (!this._delayUI) return this._socket.up(topic, msg);
    this._delayed = this._delayed.filter(x => x[0] !== topic || (isOSR?x[1].data[0] !== msg.data[0]:false));
    this._delayed.push([topic, msg]);
  };

  private setTick = () => {
    if (this._interval) clearInterval(this._interval);
    if (this._delayUI<1) this._delayUI = 0;
    this._delayed = [];
    this._interval = setInterval(
      this._delayUI ? this.onDelay : this.onTick,
      (this._delayUI || 6e1) * 1e3
    );
    this.onTick();
  };
}
