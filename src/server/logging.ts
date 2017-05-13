import bunyan = require("bunyan");
import * as _ from "lodash";

export default function log(name: string) : bunyan {
    if (process.argv.length >= 2 && _.includes(process.argv[1], "mocha"))
        return bunyan.createLogger({name: name, stream: process.stdout, level: bunyan.FATAL});

    return bunyan.createLogger({
        name: name,
        streams: [{
            level: _.includes(process.argv, 'debug') ? bunyan.DEBUG : bunyan.INFO,
            stream: process.stdout
        }]
    });
}
