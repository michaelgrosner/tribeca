import bunyan = require("bunyan");
import * as _ from "lodash";
import fs = require("fs");

export default function log(name: string) : bunyan {
    if (process.argv.length >= 2 && _.includes(process.argv[1], "mocha"))
        return bunyan.createLogger({name: name, stream: process.stdout, level: bunyan.FATAL});

    if (!fs.existsSync('./log')) fs.mkdirSync('./log');

    const level = _.includes(process.argv, 'debug') ? 'debug' : 'info';

    return bunyan.createLogger({
        name: name,
        streams: [{
            level: level,
            stream: process.stdout
        }, {
            level: level,
            path: './log/tribeca.log'
        }]
    });
}
