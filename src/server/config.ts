import fs = require("fs");

export class ConfigProvider {
    private _config: { [key: string]: string } = {};

    constructor() {
      let configFile = './etc/'+process.argv.filter(arg => arg.substr(-5)==='.json').concat('K.json')[0];

      if (fs.existsSync(configFile))
        this._config = JSON.parse(fs.readFileSync(configFile, "utf-8"));
    }

    public GetString = (configKey: string): string => {
      return this.GetDefaultString(configKey, true);
    };

    public GetDefaultString = (configKey: string, isMandatory?: boolean): string => {
      if (process.env.hasOwnProperty(configKey))
        return process.env[configKey];

      if (this._config.hasOwnProperty(configKey))
        return this._config[configKey];

      if (configKey === 'BotIdentifier' && this._config.hasOwnProperty('TRIBECA_MODE'))
        return this._config['TRIBECA_MODE']; /* delete this shit only after January 2018 */

      if (isMandatory)
        throw Error('Config does not have property ' + configKey + ', please add ' + configKey + ' to your config file (see original file etc/K.json.dist).');

      return null;
    };
}
