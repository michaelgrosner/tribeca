/// <reference path="../typings/tsd.d.ts" />

var log = require('debug');
var momentjs = require('moment');
var util = require("util");

var date = momentjs.utc;

interface Logger { (...arg : any[]) : void;}

interface Array<T> { last() : T;}
Array.prototype.last = function() { return this[this.length - 1]; };

class Evt<T> {
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