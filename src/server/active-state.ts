import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class ActiveRepository implements Interfaces.IRepository<boolean> {

    NewParameters = new Utils.Evt();

    private _savedQuotingMode: boolean = false;
    public get savedQuotingMode(): boolean {
        return this._savedQuotingMode;
    }

    private _latest: boolean = false;
    public get latest(): boolean {
        return this._latest;
    }

    constructor(startQuoting: boolean,
        private _exchangeConnectivity: Interfaces.IBrokerConnectivity,
        private _pub: Publish.IPublish<boolean>,
        private _rec: Publish.IReceive<boolean>) {
        this._savedQuotingMode = startQuoting;

        _pub.registerSnapshot(() => [this.latest]);
        _rec.registerReceiver(this.handleNewQuotingModeChangeRequest);
        _exchangeConnectivity.ConnectChanged.on(() => this.updateParameters());
    }

    private handleNewQuotingModeChangeRequest = (v: boolean) => {
        if (v !== this._savedQuotingMode) {
            this._savedQuotingMode = v;
            console.info(new Date().toISOString().slice(11, -1), 'active', 'Changed saved quoting state to', this._savedQuotingMode);
            this.updateParameters();
        }

        this._pub.publish(this.latest);
    };

    private reevaluateQuotingMode = (): boolean => {
        if (this._exchangeConnectivity.connectStatus !== Models.ConnectivityStatus.Connected) return false;
        return this._savedQuotingMode;
    };

    private updateParameters = () => {
        var newMode = this.reevaluateQuotingMode();

        if (newMode !== this._latest) {
            this._latest = newMode;
            console.log(new Date().toISOString().slice(11, -1), 'active', 'Changed quoting mode to', this.latest);
            this.NewParameters.trigger();
            this._pub.publish(this.latest);
        }
    };
}
