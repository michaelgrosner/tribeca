/// <reference path="../../typings/tsd.d.ts" />

export var log = require('debug');
import momentjs = require('moment');
import util = require("util");

export var date = momentjs.utc;

export interface Logger { (...arg : any[]) : void;}

export class Evt<T> {
    constructor(private handlers : { (data? : T): void; }[] = []) {}

    public on(handler : { (data? : T): void }) {
        this.handlers.push(handler);
    }

    public off(handler : { (data? : T): void }) {
        this.handlers = this.handlers.filter(h => h !== handler);
    }

    public trigger(data? : T) {
        for (var i = 0; i < this.handlers.length; i++) {
            this.handlers[i](data);
        }
    }
}