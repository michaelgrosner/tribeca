/// <reference path="../common/models.ts" />

import Utils = require("./utils");
import _ = require("lodash");
import fs = require("fs");
import log from "./logging";

export interface IConfigProvider {
    Has(configKey: string): boolean;

    GetString(configKey: string): string;
    GetBoolean(configKey: string): boolean;
    GetNumber(configKey: string): number;
    
    inBacktestMode: boolean;
}

export class ConfigProvider implements IConfigProvider {
    private static Log = log("tribeca:config");
    private _config: { [key: string]: string } = {};

    constructor() {
        this.inBacktestMode = (process.env["TRIBECA_BACKTEST_MODE"] || "false") === "true";
        
        const configFile = process.env["TRIBECA_CONFIG_FILE"] || "tribeca.json";
        if (fs.existsSync(configFile)) {
            this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
        }
    }

    public Has = (configKey: string): boolean => {
        return (this.Fetch(configKey, false, x => x)) !== null;
    };

    public GetNumber = (configKey: string): number => {
        return this.Fetch<number>(configKey, true, x => {
            if (typeof x === "number") return x;
            if (typeof x === "string") return parseFloat(x);
            else return parseFloat(x.toString());
        });
    };

    public GetBoolean = (configKey: string): boolean => {
        return this.Fetch<boolean>(configKey, true, x => x == true || x == "true");
    };

    public GetString = (configKey: string): string => {
        return this.Fetch<string>(configKey, true, x => x.toString());
    };
    
    private Fetch = <T>(configKey: string, throwIfMissing: boolean, cvt: (d: Object) => T): T => {
        let value : any = null;
        if (process.env.hasOwnProperty(configKey))
            value = process.env[configKey];
        else if (this._config.hasOwnProperty(configKey))
            value = this._config[configKey];
        else if (throwIfMissing)
            throw Error("Config does not have property " + configKey);

        const fetched = cvt(value);
        ConfigProvider.Log.info("%s = %s", configKey, fetched);
        return fetched;
    };
    
    inBacktestMode: boolean = false;
}