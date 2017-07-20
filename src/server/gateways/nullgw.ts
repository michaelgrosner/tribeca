import Models = require("../../share/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");

export class NullOrderGateway implements Interfaces.IOrderEntryGateway {
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => { return Promise.resolve(0); };

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => {
        return new Date().valueOf().toString().substr(-9);
    }

    private raiseTimeEvent = (o: Models.OrderStatusReport) => {
        this._evUp('OrderUpdateGateway', {
            orderId: o.orderId,
            computationalLatency: new Date().valueOf() - o.time.valueOf()
        })
    };

    sendOrder(order: Models.OrderStatusReport) {
        if (order.timeInForce == Models.TimeInForce.IOC)
            throw new Error("Cannot send IOCs");
        setTimeout(() => this.trigger(order.orderId, Models.OrderStatus.Working, order), 10);
        this.raiseTimeEvent(order);
    }

    cancelOrder(cancel: Models.OrderStatusReport) {
        setTimeout(() => this.trigger(cancel.orderId, Models.OrderStatus.Complete), 10);
        this.raiseTimeEvent(cancel);
    }

    replaceOrder(replace: Models.OrderStatusReport) {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    }

    private trigger(orderId: string, status: Models.OrderStatus, order?: Models.OrderStatusReport) {
        var rpt: Models.OrderStatusUpdate = {
            orderId: orderId,
            orderStatus: status,
            time: new Date()
        };
        this._evUp('OrderUpdateGateway', rpt);

        if (status === Models.OrderStatus.Working && Math.random() < .1) {
            var rpt: Models.OrderStatusUpdate = {
                orderId: orderId,
                orderStatus: status,
                time: new Date(),
                lastQuantity: order.quantity,
                lastPrice: order.price,
                liquidity: Math.random() < .5 ? Models.Liquidity.Make : Models.Liquidity.Take
            };
            setTimeout(() => this._evUp('OrderUpdateGateway', rpt), 1000);
        }
    }

    constructor(private _evUp) {
        setTimeout(() => this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected), 500);
    }
}

export class NullPositionGateway implements Interfaces.IPositionGateway {
    constructor(private _evUp, cfPair) {
        setInterval(() => this._evUp('PositionGateway', new Models.CurrencyPosition(500, 50, cfPair.base)), 2500);
        setInterval(() => this._evUp('PositionGateway', new Models.CurrencyPosition(500, 50, cfPair.quote)), 2500);
    }
}

export class NullMarketDataGateway implements Interfaces.IMarketDataGateway {
    constructor(private _evUp, private _minTick: number) {
        setTimeout(() => this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Connected), 500);
        setInterval(() => this._evUp('MarketDataGateway', this.generateMarketData()), 5000);
        setInterval(() => this._evUp('MarketTradeGateway', this.genMarketTrade()), 15000);
    }

    private getPrice = (sign: number) => Utils.roundNearest(1000 + sign * 100 * Math.random(), this._minTick);

    private genMarketTrade = () => {
        const side = (Math.random() > .5 ? Models.Side.Bid : Models.Side.Ask);
        const sign = Models.Side.Ask === side ? 1 : -1;
        return new Models.GatewayMarketTrade(this.getPrice(sign), Math.random(), side);
    }

    private genSingleLevel = (sign: number) => new Models.MarketSide(this.getPrice(sign), Math.random());

    private readonly Depth: number = 25;
    private generateMarketData = () => {
       const genSide = (sign: number) => {
          var s = [];
          for (var i = this.Depth;i--;) s.push(this.genSingleLevel(sign));
          return s.sort((a, b) => sign*a.price<sign*b.price?1:(sign*a.price>sign*b.price?-1:0));
       };
       return new Models.Market(genSide(-1), genSide(1));
    };
}

class NullGatewayDetails implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name(): string {
        return "Null";
    }

    makeFee(): number {
        return 0;
    }

    takeFee(): number {
        return 0;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Null;
    }

    constructor(public minTickIncrement: number, public minSize: number) {}
}

class NullGateway extends Interfaces.CombinedGateway {
    constructor(
      cfString,
      cfPair,
      _evOn,
      _evUp
    ) {
        const minTick = .01;
        new NullMarketDataGateway(_evUp, minTick);
        new NullPositionGateway(_evUp, cfPair);
        super(
            new NullOrderGateway(_evUp),
            new NullGatewayDetails(minTick, 0.01));
    }
}

export async function createNullGateway(cfString, cfPair, _evOn, _evUp) : Promise<Interfaces.CombinedGateway> {
    return new NullGateway(cfString, cfPair, _evOn, _evUp);
}
