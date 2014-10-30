/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />
/// <reference path="null.ts" />

module OkCoin {
    class OkCoinMarketDataGateway implements IMarketDataGateway {
        MarketData = new Evt<MarketBook>();
        ConnectChanged = new Evt<ConnectivityStatus>();

        constructor(config : IConfigProvider) {}
    }

    class OkCoinOrderEntryGateway implements IOrderEntryGateway {
        OrderUpdate = new Evt<OrderStatusReport>();
        ConnectChanged = new Evt<ConnectivityStatus>();

        sendOrder = (order : BrokeredOrder) : OrderGatewayActionReport => {
            return undefined;
        };

        cancelOrder = (cancel : BrokeredCancel) : OrderGatewayActionReport => {
            return undefined;
        };

        replaceOrder = (replace : BrokeredReplace) : OrderGatewayActionReport => {
            return undefined;
        };

        constructor(config : IConfigProvider) {}
    }

    class OkCoinPositionGateway implements IPositionGateway {
        PositionUpdate = new Evt<CurrencyPosition>();

        constructor(config : IConfigProvider) {}
    }

    class OkCoinBaseGateway implements IExchangeDetailsGateway {
        name() : string {
            return "OkCoin";
        }

        makeFee() : number {
            return 0.0005;
        }

        takeFee() : number {
            return 0.002;
        }

        exchange() : Exchange {
            return Exchange.OkCoin;
        }

    }

    export class OkCoin extends CombinedGateway {
        constructor(config : IConfigProvider) {
                super(
                    new OkCoinMarketDataGateway(config),
                    new OkCoinOrderEntryGateway(config),
                    new OkCoinPositionGateway(config),
                    new OkCoinBaseGateway());
            }
    }
}

