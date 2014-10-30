/// <reference path="typings/tsd.d.ts" />

var log = require('debug');
var momentjs = require('moment');
var util = require("util");

var date = momentjs.utc;

interface Logger { (...arg : any[]) : void;}

interface Array<T> { last() : T;}
Array.prototype.last = function() { return this[this.length - 1]; };

class Evt<T> {
    constructor(private handlers : { (data? : T): void; }[] = []) {}

    public on(handler : { (data? : T): void }) {
        this.handlers.push(handler);
    }

    public off(handler : { (data? : T): void }) {
        this.handlers = this.handlers.filter(h => h !== handler);
    }

    public trigger(data? : T) {
        for (var i = 0; i < this.handlers.length; i++) {
            this.handlers[i](data);
        }
    }
}

class BaseConfig {
    public static get AtlasAtsSimpleToken() : string { return "9464b821cea0d62939688df750547593"; }
    public static get AtlasAtsAccount() : string { return "1352"; }
    public static get AtlasAtsSecret() : string { return "d61eb29445f7a72a83fbc056b1693c962eb97524918f1e9e2d10b6965c16c8c7"; }
    public static get AtlasAtsMultiToken() : string { return "0e48f9bd6f8dec728df2547b7a143e504a83cb2d"; }
    public static get AtlasAtsHttpUrl() : string { return "https://atlasats.com"; }
    public static get MaxSize() : string { return "0.01"; }
}

class ProdConfig {
    public static get HitBtcPullUrl() : string { return "http://api.hitbtc.com"; }
    public static get HitBtcOrderEntryUrl() : string { return "wss://api.hitbtc.com:8080"; }
    public static get HitBtcMarketDataUrl() : string { return 'ws://api.hitbtc.com:80'; }
    public static get HitBtcApiKey() : string { return '5565ab3621cb0f8f9f07a926ce46ef2c'; }
    public static get HitBtcSecret() : string { return "b03fe2dad1c1843510edcca56446ac20"; }

    public static get AtlasAtsWsUrl() : string { return 'wss://atlasats.com/api/v1/streaming'; }
}

class DebugConfig {
    public static get HitBtcPullUrl() : string { return "http://demo-api.hitbtc.com"; }
    public static get HitBtcOrderEntryUrl() : string { return "ws://demo-api.hitbtc.com:8080"; }
    public static get HitBtcMarketDataUrl() : string { return 'ws://demo-api.hitbtc.com:80'; }
    public static get HitBtcApiKey() : string { return '004ee1065d6c7a6ac556bea221cd6338'; }
    public static get HitBtcSecret() : string { return "aa14d615df5d47cb19a13ffe4ea638eb"; }

    public static get AtlasAtsWsUrl() : string { return 'ws://test-atlasats.com/api/v1/streaming'; }
}

interface IConfigProvider {
    GetString(configKey : string) : string;
    GetNumber(configKey : string) : number;
}

class ConfigProvider implements IConfigProvider {
    private static Log : Logger = log("tribeca:config");
    private _mode : string = process.env.TRIBECA_MODE;
    private _config : {[key: string] : string} = {};

    constructor() {
        this._mode = process.env.TRIBECA_MODE;

        var _configOverrideSet : any;
        if (this._mode == "prod") {
            _configOverrideSet = ProdConfig;
        }
        else if (this._mode == "dev") {
            _configOverrideSet = DebugConfig;
        }
        else {
            throw Error(this._mode + " is not a valid TRIBECA_MODE");
        }

        for (var k in BaseConfig) {
            if (BaseConfig.hasOwnProperty(k))
                this._config[k] = BaseConfig[k];
        }

        for (var k in _configOverrideSet) {
            if (_configOverrideSet.hasOwnProperty(k))
                this._config[k] = _configOverrideSet[k];
        }

        for (var k in this._config) {
            if (this._config.hasOwnProperty(k)) {
                ConfigProvider.Log("%s = %s (%s)", k, this._config[k], _configOverrideSet.hasOwnProperty(k) ? this._mode : "base");
            }
        }
    }

    public GetNumber = (configKey : string) : number => {
        return parseFloat(this.GetString(configKey));
    };

    public GetString = (configKey : string) : string => {
        if (this._config.hasOwnProperty(configKey))
            return this._config[configKey];

        if (BaseConfig.hasOwnProperty(configKey))
            return BaseConfig[configKey];

        throw Error(this._mode + " config does not have property " + configKey);
    };
}