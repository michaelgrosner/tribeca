import bunyan = require("bunyan");
import * as _ from "lodash";

export default function log(name: string) : bunyan {
    // don't log while testing
    const isRunFromMocha = process.argv.length >= 2 && _.includes(process.argv[1], "mocha");
    if (isRunFromMocha) {
        return bunyan.createLogger({name: name, stream: process.stdout, level: bunyan.FATAL});
    }

    let level = "info";
    if (_.includes(process.argv, "debug")) {
        level = "debug";
    }

    return bunyan.createLogger({
        name: name,
        streams: [{ level: level, stream: process.stdout }]
    });
}