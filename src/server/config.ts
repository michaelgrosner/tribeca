import fs = require("fs");

export interface IConfigProvider {
    GetString(configKey: string): string;
    GetNumber(configKey: string): number;

    inBacktestMode: boolean;
}

export class ConfigProvider implements IConfigProvider {
    private _config: { [key: string]: string } = {};

    constructor() {
        this.inBacktestMode = (process.env["TRIBECA_BACKTEST_MODE"] || "false") === "true";

        let configFile = './etc/'+process.argv.filter(arg => arg.substr(-5)==='.json').concat('tribeca.json')[0];

        if (fs.existsSync(configFile))
            this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
    }

    public GetNumber = (configKey: string): number => {
        return parseFloat(this.GetString(configKey));
    };

    public GetString = (configKey: string): string => {
        var value = this.Fetch(configKey);
        return value;
    };

    private Fetch = (configKey: string): string => {
        if (process.env.hasOwnProperty(configKey))
            return process.env[configKey];

        if (this._config.hasOwnProperty(configKey))
            return this._config[configKey];

        throw Error('Config does not have property ' + configKey + ', please add ' + configKey + ' to your config file (see original file etc/tribeca.json.dist).');
    };

    inBacktestMode: boolean = false;
}
