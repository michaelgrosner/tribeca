import * as Models from "../common/models";
import _ = require("lodash");
import {ITimeProvider} from "./utils";

const getOrFallback = <T>(n: T|undefined, o: T) => typeof n !== "undefined" ? n : o;

export class OrderStateProcessor {
    constructor(
        private readonly _timeProvider: ITimeProvider, 
        private readonly makeFee : number, 
        private readonly takeFee: number) {}

    public applyOrderUpdate = (osr: Models.OrderStatusUpdate, orig: Models.OrderStatusReport) : [Models.OrderStatusReport, Models.Trade] => {
        const quantity = getOrFallback(osr.quantity, orig.quantity);

        let cumQuantity : number;
        if (!_.isUndefined(osr.lastQuantity) && _.isUndefined(osr.cumQuantity)) {
            cumQuantity = osr.lastQuantity + (orig.cumQuantity || 0);
        }
        cumQuantity = osr.cumQuantity || cumQuantity;

        let leavesQuantity : number;
        if (!_.isUndefined(osr.lastQuantity) && _.isUndefined(osr.leavesQuantity)) {
            leavesQuantity = quantity - cumQuantity;
        }
        leavesQuantity = osr.leavesQuantity || leavesQuantity;

        const partiallyFilled = cumQuantity > 0 && cumQuantity !== quantity;

        let version = 0;
        let openQtyChange = 0;
        if (typeof orig.version === "undefined") {
            openQtyChange = quantity;
        }
        else {
            const chg = leavesQuantity - orig.leavesQuantity;
            if (chg !== orig.openQtyChange) openQtyChange = chg;
            version = 1 + orig.version;
        }

        const o : Models.OrderStatusReport = {
            pair: getOrFallback(osr.pair, orig.pair),
            side: getOrFallback(osr.side, orig.side),
            quantity: quantity,
            type: getOrFallback(osr.type, orig.type),
            price: getOrFallback(osr.price, orig.price),
            timeInForce: getOrFallback(osr.timeInForce, orig.timeInForce),
            orderId: getOrFallback(osr.orderId, orig.orderId),
            exchangeId: getOrFallback(osr.exchangeId, orig.exchangeId),
            orderStatus: getOrFallback(osr.orderStatus, orig.orderStatus),
            rejectMessage: osr.rejectMessage,
            time: getOrFallback(osr.time, this._timeProvider.utcNow()),
            lastQuantity: osr.lastQuantity,
            lastPrice: osr.lastPrice,
            leavesQuantity: leavesQuantity,
            cumQuantity: cumQuantity,
            averagePrice: osr.averagePrice || orig.averagePrice,
            liquidity: getOrFallback(osr.liquidity, orig.liquidity),
            exchange: getOrFallback(osr.exchange, orig.exchange),
            computationalLatency: getOrFallback(osr.computationalLatency, 0) + getOrFallback(orig.computationalLatency, 0),
            version: version,
            partiallyFilled: partiallyFilled,
            pendingCancel: osr.pendingCancel,
            pendingReplace: osr.pendingReplace,
            cancelRejected: osr.cancelRejected,
            preferPostOnly: getOrFallback(osr.preferPostOnly, orig.preferPostOnly),
            source: getOrFallback(osr.source, orig.source),
            openQtyChange: openQtyChange
        };

        let trade : Models.Trade;
        if (osr.lastQuantity > 0) {
            let value = Math.abs(o.lastPrice * o.lastQuantity);

            const liq = o.liquidity;
            let feeCharged = null;
            if (typeof liq !== "undefined") {
                // negative fee is a rebate, positive fee is a fee
                feeCharged = (liq === Models.Liquidity.Make ? this.makeFee : this.takeFee);
                const sign = (o.side === Models.Side.Bid ? 1 : -1);
                value = value * (1 + sign * feeCharged);
            }

            trade = new Models.Trade(o.orderId+"."+o.version, o.time, o.exchange, o.pair, 
                o.lastPrice, o.lastQuantity, o.side, value, o.liquidity, feeCharged);
        }

        return [o, trade];
    };
}

