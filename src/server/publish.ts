import Models = require("../share/models");
import Monitor = require("./monitor");

export class Publisher {
    private _snapshot: () => any[] = null;
    private _lastMarketData: number = new Date().getTime();
    constructor(
      private topic: string,
      private _io: SocketIO.Server,
      private _monitor?: Monitor.ApplicationState
    ) {
        this.registerSnapshot(null);

        var onConnection = s => {
            s.on(Models.Prefixes.SUBSCRIBE + topic, () => {
                if (this._snapshot !== null) {
                  let snap: any[] = this._snapshot();
                  if (this.topic === Models.Topics.MarketData)
                      snap = this.compressSnapshot(this._snapshot(), this.compressMarketDataInc);
                  else if (this.topic === Models.Topics.OrderStatusReports)
                    snap = this.compressSnapshot(this._snapshot(), this.compressOSRInc);
                  else if (this.topic === Models.Topics.Position)
                    snap = this.compressSnapshot(this._snapshot(), this.compressPositionInc);
                  s.emit(Models.Prefixes.SNAPSHOT + topic, snap);
                }
            });
        };

        this._io.on("connection", onConnection);

        Object.keys(this._io.sockets.connected).forEach(s => {
            onConnection(this._io.sockets.connected[s]);
        });
    }

    public publish = (msg: any) => {
      if (this.topic === Models.Topics.MarketData) {
        if (this._lastMarketData+369>new Date().getTime()) return;
        msg = this.compressMarketDataInc(msg);
      } else if (this.topic === Models.Topics.OrderStatusReports)
        msg = this.compressOSRInc(msg);
      else if (this.topic === Models.Topics.Position)
        msg = this.compressPositionInc(msg);
      if (this._monitor)
        this._monitor.delay(Models.Prefixes.MESSAGE, this.topic, msg)
      else this._io.emit(Models.Prefixes.MESSAGE + this.topic, msg);
    };

    public registerSnapshot = (generator: () => any[]) => {
        if (this._snapshot === null) this._snapshot = generator;
        else throw new Error("already registered snapshot generator for topic " + this.topic);
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
