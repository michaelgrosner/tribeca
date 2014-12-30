/// <reference path="../../typings/tsd.d.ts" />

export class Timestamped<T> {
    constructor(public data : T, public time : Moment) {}

    public toString() {
        return "time=" + toUtcFormattedTime(this.time) + ";data=" + this.data;
    }
}

export class MarketSide {
    constructor(public price: number, public size: number) { }

    public equals(other : MarketSide) {
        return this.price == other.price && this.size == other.size;
    }

    public toString() {
        return "price=" + this.price + ";size=" + this.size;
    }
 }

export class MarketUpdate {
    constructor(
        public bid : MarketSide,
        public ask : MarketSide,
        public time : Moment) { }

    public equals(other : MarketUpdate) {
        if (other == null) return false;
        return this.ask.equals(other.ask) && this.bid.equals(other.bid);
    }

    public toString() {
        return "bid=[" + this.bid + "];ask=[" + this.ask + "];time=" + this.time.format('M/d/YY h:mm:ss,SSS');
    }
}

export class Market {
    constructor(
        public pair : CurrencyPair,
        public update : MarketUpdate,
        public exchange : Exchange,
        public flag : MarketDataFlag) { }

    public toString() {
        return this.pair + ";" + this.update + ";exchange=" + Exchange[this.exchange] + ";flag=" + MarketDataFlag[this.flag];
    }
}

export enum GatewayType { MarketData, OrderEntry, Position }
export enum Currency { USD, BTC, LTC }
export enum ConnectivityStatus { Connected, Disconnected }
export enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts, BtcChina }
export enum Side { Bid, Ask }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
export enum Liquidity { Make, Take }

export enum MarketDataFlag {
    NoChange = 0,
    First = 1,
    PriceChanged = 1 << 1,
    SizeChanged = 1 << 2,
    PriceAndSizeChanged = 1 << 3
}

export interface Order {
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
    exchange : Exchange;
}

export class SubmitNewOrder implements Order {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange,
        public generatedTime: Moment,
        public msg: string = null) {}
}

export class CancelReplaceOrder {
    constructor(
        public origOrderId : string,
        public quantity : number,
        public price : number,
        public exchange : Exchange,
        public generatedTime : Moment) {}
}

export class OrderCancel {
    constructor(
        public origOrderId : string,
        public exchange : Exchange,
        public generatedTime : Moment) {}
}

export class BrokeredOrder implements Order {
    constructor(
        public orderId : string,
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange) {}
}

export class BrokeredReplace implements Order {
    constructor(
        public orderId : string,
        public origOrderId : string,
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange,
        public exchangeId : string) {}
}

export class BrokeredCancel {
    constructor(
        public clientOrderId : string,
        public requestId : string,
        public side : Side,
        public exchangeId : string) {}
}

export class SentOrder {
    constructor(public sentOrderClientId : string) {}
}

export class OrderGatewayActionReport {
    constructor(public sentTime : Moment) {}
}

export interface OrderStatusReport {
    side? : Side;
    quantity? : number;
    type? : OrderType;
    price? : number;
    timeInForce? : TimeInForce;
    orderId? : string;
    exchangeId? : string;
    orderStatus? : OrderStatus;
    rejectMessage? : string;
    time? : Moment;
    lastQuantity? : number;
    lastPrice? : number;
    leavesQuantity? : number;
    cumQuantity? : number;
    averagePrice? : number;
    liquidity? : Liquidity;
    exchange? : Exchange;
    computationalLatency? : number;
    version? : number;

    partiallyFilled? : boolean;
    pendingCancel? : boolean;
    pendingReplace? : boolean;
    cancelRejected? : boolean;
}

export class OrderStatusReportImpl implements OrderStatusReport {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public orderId : string,
        public exchangeId : string,
        public orderStatus : OrderStatus,
        public rejectMessage : string,
        public time : Moment,
        public lastQuantity : number,
        public lastPrice : number,
        public leavesQuantity : number,
        public cumQuantity : number,
        public averagePrice : number,
        public liquidity : Liquidity,
        public exchange : Exchange,
        public computationalLatency : number,
        public version : number,
        public partiallyFilled : boolean,
        public pendingCancel : boolean,
        public pendingReplace : boolean,
        public cancelRejected : boolean) {}

    public toString() {
        var components : string[] = [];

        components.push("orderId="+this.orderId);
        components.push("time="+this.time.format('M/d/YY h:mm:ss,SSS'));
        if (this.exchangeId != null) components.push("exchangeId="+this.exchangeId);
        if (this.exchange != null) components.push("exchange="+Exchange[this.exchange]);
        components.push("orderStatus="+OrderStatus[this.orderStatus]);
        components.push("side="+Side[this.side]);
        components.push("quantity="+this.quantity);
        components.push("price="+this.price);
        components.push("tif="+TimeInForce[this.timeInForce]);
        components.push("type="+OrderType[this.type]);
        components.push("version="+this.version);
        if (this.rejectMessage) components.push(this.rejectMessage);
        if (this.computationalLatency != null) components.push("computationalLatency="+this.computationalLatency);
        if (this.lastQuantity != null) components.push("lastQuantity="+this.lastQuantity);
        if (this.lastPrice != null) components.push("lastPrice="+this.lastPrice);
        if (this.leavesQuantity != null) components.push("leavesQuantity="+this.leavesQuantity);
        if (this.cumQuantity != null) components.push("cumQuantity="+this.cumQuantity);
        if (this.averagePrice != null) components.push("averagePrice="+this.averagePrice);
        if (this.liquidity != null) components.push("liquidity="+Liquidity[this.liquidity]);
        if (this.partiallyFilled) components.push("partiallyFilled");
        if (this.pendingCancel) components.push("pendingCancel");
        if (this.pendingReplace) components.push("pendingReplace");
        if (this.cancelRejected) components.push("cancelRejected");

        return components.join(";");
    }
}

export class ExchangeCurrencyPosition {
    constructor(public amount : number,
                public currency : Currency,
                public exchange : Exchange) { }

    public toString() {
        return "exchange=" + Exchange[this.exchange] + ";currency=" + Currency[this.currency] + ";amount=" + this.amount;
    }
}

export class CurrencyPosition {
    constructor(public amount : number,
                public currency : Currency) {}

    public toExchangeReport(exch : Exchange) {
        return new ExchangeCurrencyPosition(this.amount, this.currency, exch);
    }

    public toString() {
        return "currency=" + Currency[this.currency] + ";amount=" + this.amount;
    }
}

export class OrderRequestFromUI {
    constructor(
        public exchange : string,
        public side : string,
        public price : number,
        public quantity : number,
        public timeInForce : string,
        public orderType : string) {}
}

export interface ReplaceRequestFromUI {
    price : number;
    quantity : number;
}

export class FairValue {
    constructor(public price : number,
                public mkt : Market) {}
}

export class TradingDecision {
    constructor(public bidAction : QuoteSent, public bidQuote : Quote,
                public askAction : QuoteSent, public askQuote : Quote,
                public fairValue : FairValue) {}

    public toString() {
        return "bid:[" + this.bidQuote + "::" + QuoteSent[this.bidAction] + "], " +
               "ask:[" + this.askQuote + "::" + QuoteSent[this.askAction] + "], " +
               "fv: " + this.fairValue.price;
    }
}

export enum QuoteAction { New, Cancel }
export enum QuoteSent { First, Modify, UnsentDuplicate, Delete, UnableToSend }

export class Quote {
    constructor(public type : QuoteAction,
                public side : Side,
                public exchange : Exchange,
                public time : Moment,
                public price : number = null,
                public size : number = null) {}

    public equals(other : Quote, tol : number = 1e-3) {
        return this.type == other.type
            && this.side == other.side
            && this.exchange == other.exchange
            && Math.abs(this.price - other.price) < tol
            && Math.abs(this.size - other.size) < tol;
    }

    public toString() {
        if (this.type == QuoteAction.New) {
            return "px="+this.price+";sz="+this.size;
        }
        else {
            return "del";
        }
    }
}

export class CurrencyPair {
    constructor(public base : Currency, public quote : Currency) {}

    public toString() {
        return CurrencyPair[this.base] + "/" + CurrencyPair[this.quote];
    }
}

export function toUtcFormattedTime(t : Moment) {
    return t.format('M/D/YY HH:mm:ss,SSS');
}