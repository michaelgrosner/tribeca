"use strict";
var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.QuoteCurrencyCellComponent = exports.BaseCurrencyCellComponent = void 0;
const core_1 = require("@angular/core");
let BaseCurrencyCellComponent = class BaseCurrencyCellComponent {
    constructor() {
        this.productFixedSize = 8;
    }
    agInit(params) {
        this.params = params;
        if ('productFixedSize' in params.node.data)
            this.productFixedSize = params.node.data.productFixedSize;
    }
    refresh() {
        return false;
    }
};
BaseCurrencyCellComponent = __decorate([
    core_1.Component({
        selector: 'base-currency-cell',
        template: `{{ (params.value||0).toFixed(productFixedSize) }}`
    })
], BaseCurrencyCellComponent);
exports.BaseCurrencyCellComponent = BaseCurrencyCellComponent;
let QuoteCurrencyCellComponent = class QuoteCurrencyCellComponent {
    constructor() {
        this.quoteSymbol = 'USD';
        this.productFixedPrice = 8;
    }
    agInit(params) {
        this.params = params;
        if ('quoteSymbol' in params.node.data)
            this.quoteSymbol = params.node.data.quoteSymbol.substr(0, 3);
        if ('productFixedPrice' in params.node.data)
            this.productFixedPrice = params.node.data.productFixedPrice;
    }
    refresh() {
        return false;
    }
};
QuoteCurrencyCellComponent = __decorate([
    core_1.Component({
        selector: 'quote-currency-cell',
        template: `{{ (params.value||0).toFixed(productFixedPrice) }} {{ quoteSymbol }}`
    })
], QuoteCurrencyCellComponent);
exports.QuoteCurrencyCellComponent = QuoteCurrencyCellComponent;
