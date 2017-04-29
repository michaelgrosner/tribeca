"use strict";
exports.__esModule = true;
var Utils = require("./utils");
var fs = require("fs");
var ConfigProvider = (function () {
    function ConfigProvider() {
        var _this = this;
        this._config = {};
        this.GetNumber = function (configKey) {
            return parseFloat(_this.GetString(configKey));
        };
        this.GetString = function (configKey) {
            var value = _this.Fetch(configKey);
            // ConfigProvider.Log.info("%s = %s", configKey, value);
            return value;
        };
        this.Fetch = function (configKey) {
            if (process.env.hasOwnProperty(configKey))
                return process.env[configKey];
            if (_this._config.hasOwnProperty(configKey))
                return _this._config[configKey];
            throw Error("Config does not have property " + configKey);
        };
        this.inBacktestMode = false;
        this.inBacktestMode = (process.env["TRIBECA_BACKTEST_MODE"] || "false") === "true";
        var configFile = process.env["TRIBECA_CONFIG_FILE"] || "./etc/tribeca.json";
        if (fs.existsSync(configFile)) {
            this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
        }
    }
    return ConfigProvider;
}());
ConfigProvider.Log = Utils.log("tribeca:config");
exports.ConfigProvider = ConfigProvider;
