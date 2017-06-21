import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Broker = require("./broker");

export class ActiveRepository {

    ExchangeConnectivity = new Utils.Evt();

    private _savedQuotingMode: boolean = false;
    public get savedQuotingMode(): boolean {
        return this._savedQuotingMode;
    }

    private _latest: boolean = false;
    public get latest(): boolean {
        return this._latest;
    }

    constructor(startQuoting: boolean,
        private _exchangeConnectivity: Broker.ExchangeBroker,
        private _publisher: Publish.Publisher,
        private _reciever: Publish.Receiver) {
        this._savedQuotingMode = startQuoting;

        _publisher.registerSnapshot(Models.Topics.ActiveChange, () => [this.latest]);
        _reciever.registerReceiver(Models.Topics.ActiveChange, this.handleNewQuotingModeChangeRequest);
        _exchangeConnectivity.ConnectChanged.on(() => this.updateConnectivity());
    }

    private handleNewQuotingModeChangeRequest = (v: boolean) => {
        if (v !== this._savedQuotingMode) {
            this._savedQuotingMode = v;
            this.updateConnectivity();
        }

        this._publisher.publish(Models.Topics.ActiveChange, this.latest);
    };

    private reevaluateQuotingMode = (): boolean => {
        if (this._exchangeConnectivity.connectStatus !== Models.ConnectivityStatus.Connected) return false;
        return this._savedQuotingMode;
    };

    private updateConnectivity = () => {
        var newMode = this.reevaluateQuotingMode();

        if (newMode !== this._latest) {
            this._latest = newMode;
            console.log(new Date().toISOString().slice(11, -1), 'active', 'Changed quoting mode to', this.latest);
            this.ExchangeConnectivity.trigger();
            this._publisher.publish(Models.Topics.ActiveChange, this.latest);
        }
    };
}
