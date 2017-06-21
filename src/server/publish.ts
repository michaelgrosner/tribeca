import Models = require("../share/models");
import Monitor = require("./monitor");

export class Publisher {
  private _snapshot: boolean[] = [];
  private _lastMarketData: number = new Date().getTime();
  public monitor: Monitor.ApplicationState;
  constructor(
    private _io: SocketIO.Server,
  ) {
  }

  public publish = (topic: string, msg: any, monitor?: boolean) => {
    if (topic === Models.Topics.MarketData) {
      if (this._lastMarketData+369>new Date().getTime()) return;
      msg = this.compressMarketDataInc(msg);
    } else if (topic === Models.Topics.OrderStatusReports)
      msg = this.compressOSRInc(msg);
    else if (topic === Models.Topics.Position)
      msg = this.compressPositionInc(msg);
    if (monitor && this.monitor)
      this.monitor.delay(Models.Prefixes.MESSAGE, topic, msg)
    else this._io.emit(Models.Prefixes.MESSAGE + topic, msg);
  };

  public registerSnapshot = (topic: string, snapshot: () => any[]) => {
    if (typeof this._snapshot[topic] !== 'undefined')
      throw new Error("Already registered snapshot for topic " + topic);

    this._snapshot[topic] = true;

    this._io.on("connection", s => {
      s.on(Models.Prefixes.SUBSCRIBE + topic, () => {
        let snap: any[] = snapshot();
        if (topic === Models.Topics.MarketData)
            snap = this.compressSnapshot(snapshot(), this.compressMarketDataInc);
        else if (topic === Models.Topics.OrderStatusReports)
          snap = this.compressSnapshot(snapshot(), this.compressOSRInc);
        else if (topic === Models.Topics.Position)
          snap = this.compressSnapshot(snapshot(), this.compressPositionInc);
        s.emit(Models.Prefixes.SNAPSHOT + topic, snap);
      });
    });

    return this;
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
      data.exchange,
      data.price,
      data.quantity,
      data.side,
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
}

export class Receiver {
  private _handler: boolean[] = [];

  constructor(
    private _io: SocketIO.Server
  ) {
  }

  public registerReceiver = (topic: string, handler : (msg : any) => void) => {
    if (typeof this._handler[topic] !== 'undefined')
      throw new Error("already registered receive handler for topic " + topic);

    this._handler[topic] = true;

    this._io.on("connection", (s: SocketIO.Socket) => {
      s.on(Models.Prefixes.MESSAGE + topic, msg => {
          handler(msg);
      });
    });
  };
}
