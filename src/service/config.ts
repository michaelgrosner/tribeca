/// <reference path="../common/models.ts" />

import Utils = require("./utils");
import _ = require("lodash");
import fs = require("fs");

export interface IConfigProvider {
    GetString(configKey: string): string;
    GetNumber(configKey: string): number;
    
    inBacktestMode: boolean;
}

export class ConfigProvider implements IConfigProvider {
    private static Log = Utils.log("tribeca:config");
    private _config: { [key: string]: string } = {};

    constructor() {
        this.inBacktestMode = (process.env["TRIBECA_BACKTEST_MODE"] || "false") === "true";
        
        var configFile = process.env["TRIBECA_CONFIG_FILE"] || "tribeca.json";
        if (fs.existsSync(configFile)) {
            this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
        }
    }

    public GetNumber = (configKey: string): number => {
        return parseFloat(this.GetString(configKey));
    };

    public GetString = (configKey: string): string => {
        var value = this.Fetch(configKey);
        ConfigProvider.Log.info("%s = %s", configKey, value);
        return value;
    };
    
    private Fetch = (configKey: string): string => {
        if (process.env.hasOwnProperty(configKey))
            return process.env[configKey];
        
        if (this._config.hasOwnProperty(configKey))
            return this._config[configKey];

        throw Error("Config does not have property " + configKey);
    };
    
    inBacktestMode: boolean = false;
}