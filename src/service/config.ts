/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />

import Utils = require("./utils");
import _ = require("lodash");
import fs = require("fs");

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
        
        if (process.env.hasOwnProperty("TRIBECA_MODE") && process.env.TRIBECA_MODE === "backtest") {
            this.inBacktestMode = true;
        }
        else {
            var configFile = process.env.TRIBECA_CONFIG_FILE || "tribeca.json";
            this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
        }
    }

    public GetNumber = (configKey: string): number => {
        return parseFloat(this.GetString(configKey));
    };

    public GetString = (configKey: string): string => {
        return this.Fetch(configKey);
    };
    
    private FetchAndLog = (configKey: string) => { 
        var value = this.Fetch(configKey);
        ConfigProvider.Log("%s = %s", configKey, value);
        return value;
    };
    
    private Fetch = (configKey: string) => {
        if (process.env.hasOwnProperty(configKey))
            return process.env[configKey];
        
        if (this._config.hasOwnProperty(configKey))
            return this._config[configKey];

        throw Error(Environment[this._env] + " config does not have property " + configKey);
    };
    
    inBacktestMode: boolean = false;
}