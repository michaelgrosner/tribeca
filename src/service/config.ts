/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />

import Utils = require("./utils");
import _ = require("lodash");

class ProdConfig {
    public static get HitBtcPullUrl(): string { return "http://api.hitbtc.com"; }
    public static get HitBtcOrderEntryUrl(): string { return "wss://api.hitbtc.com:8080"; }
    public static get HitBtcMarketDataUrl(): string { return 'ws://api.hitbtc.com:80'; }
    public static get HitBtcApiKey(): string { return 'NULL'; }
    public static get HitBtcSecret(): string { return "NULL"; }
    public static get HitBtcSocketIoUrl(): string { return "https://api.hitbtc.com:8081"; }

    public static get CoinbasePassphrase() { return "NULL"; }
    public static get CoinbaseApiKey() { return "NULL"; }
    public static get CoinbaseSecret() { return "NULL"; }
    public static get CoinbaseRestUrl() { return "https://api.exchange.coinbase.com"; }
    public static get CoinbaseWebsocketUrl() { return "wss://ws-feed.exchange.coinbase.com"; }

    public static get CoinbaseOrderDestination(): string { return "Coinbase"; }
    public static get HitBtcOrderDestination(): string { return "HitBtc"; }
}

class DebugConfig {
    public static get HitBtcPullUrl(): string { return "http://demo-api.hitbtc.com"; }
    public static get HitBtcOrderEntryUrl(): string { return "ws://demo-api.hitbtc.com:8080"; }
    public static get HitBtcMarketDataUrl(): string { return 'ws://demo-api.hitbtc.com:80'; }
    public static get HitBtcApiKey(): string { return 'NULL'; }
    public static get HitBtcSecret(): string { return "NULL"; }
    public static get HitBtcSocketIoUrl(): string { return "https://demo-api.hitbtc.com:8081"; }
    
    public static get CoinbasePassphrase() { return "NULL"; }
    public static get CoinbaseApiKey() { return "NULL"; }
    public static get CoinbaseSecret() { return "NULL"; }
    public static get CoinbaseRestUrl() { return "https://api-public.sandbox.exchange.coinbase.com"; }
    public static get CoinbaseWebsocketUrl() { return "https://ws-feed-public.sandbox.exchange.coinbase.com"; }

    public static get CoinbaseOrderDestination(): string { return "Coinbase"; }
    public static get HitBtcOrderDestination(): string { return "HitBtc"; }
}

export enum Environment {
    Dev,
    Prod
}

export interface IConfigProvider {
    GetString(configKey: string): string;
    GetNumber(configKey: string): number;

    environment(): Environment;
    inBacktestMode: boolean;
}

export class ConfigProvider implements IConfigProvider {
    private _env: Environment;
    environment(): Environment {
        return this._env;
    }

    private static Log: Utils.Logger = Utils.log("tribeca:config");
    private _config: { [key: string]: string } = {};

    constructor() {
        var _configOverrideSet: any;

        var rawEnv = process.env.TRIBECA_MODE;
        switch (rawEnv) {
            case "prod":
                _configOverrideSet = ProdConfig;
                this._env = Environment.Prod;
                break;
            case "dev":
                _configOverrideSet = DebugConfig;
                this._env = Environment.Dev;
                break;
            case "backtest":
                _configOverrideSet = DebugConfig;
                this._env = Environment.Dev;
                this.inBacktestMode = true;
                break;
            default:
                throw Error(this._env + " is not a valid TRIBECA_MODE");
        }

        for (var k in _configOverrideSet) {
            if (_configOverrideSet.hasOwnProperty(k))
                this._config[k] = _configOverrideSet[k];
        }

        if (!this.inBacktestMode) {
            for (var k in this._config) {
                if (this._config.hasOwnProperty(k)) {
                    var value = _configOverrideSet.hasOwnProperty(k) ? Environment[this._env] : "base";
                    ConfigProvider.Log("%s = %s (%s)", k, this._config[k], value);
                }
            }
        }
    }

    public GetNumber = (configKey: string): number => {
        return parseFloat(this.GetString(configKey));
    };

    public GetString = (configKey: string): string => {
        if (this._config.hasOwnProperty(configKey))
            return this._config[configKey];

        throw Error(Environment[this._env] + " config does not have property " + configKey);
    };
    
    inBacktestMode: boolean = false;
}