/// <reference path="typings/tsd.d.ts" />

var log = require('debug');

interface Logger { (...arg : any[]) : void; }

class Evt<T> {
    private handlers: { (data?: T): void; }[] = [];

    public on(handler: { (data?: T): void }) {
        this.handlers.push(handler);
    }

    public off(handler: { (data?: T): void }) {
        this.handlers = this.handlers.filter(h => h !== handler);
    }

    public trigger(data?: T) {
        if (this.handlers) {
            this.handlers.forEach(h => h(data));
        }
    }
}