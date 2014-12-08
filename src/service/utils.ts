/// <reference path="../../typings/tsd.d.ts" />

import momentjs = require('moment');
export var date = momentjs.utc;

import util = require("util");
import winston = require("winston");
winston.add(winston.transports.File, { filename: 'tribeca.log', timestamp: false, json: false });
export var log = (name : string) => {
    return (...msg : any[]) => {
        var head = util.format.bind(this, date().format('M/d/YY h:mm:ss,SSS') + "\t[" + name + "]\t" + msg.shift());
        winston.info(head.apply(this, msg));
    };
};
export interface Logger { (...arg : any[]) : void;}

export class Evt<T> {
    constructor(private handlers : { (data? : T): void; }[] = []) {}

    public on(handler : (data? : T) => void) {
        this.handlers.push(handler);
    }

    public off(handler : (data? : T) => void) {
        this.handlers = this.handlers.filter(h => h !== handler);
    }

    public trigger(data? : T) {
        for (var i = 0; i < this.handlers.length; i++) {
            this.handlers[i](data);
        }
    }
}