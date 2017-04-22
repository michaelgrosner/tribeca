/// <reference path="../../common/models.ts" />

import StyleHelpers = require("./helpers");
import Models = require("../../common/models");
import _ = require("lodash");

class NullQuoteGenerator implements StyleHelpers.QuoteStyle {
	Mode = null;
    
    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote|null => {
		return null;
	};
}

export class QuotingStyleRegistry {
	private _mapping : StyleHelpers.QuoteStyle[];
	
	constructor(modules: StyleHelpers.QuoteStyle[]) {
		this._mapping = _.sortBy(modules, s => s.Mode);
	}
	
	private static NullQuoteGenerator : StyleHelpers.QuoteStyle = new NullQuoteGenerator();
	
	public Get = (mode : Models.QuotingMode) : StyleHelpers.QuoteStyle => {
		var mod = this._mapping[mode];
		
		if (typeof mod === "undefined")
			return QuotingStyleRegistry.NullQuoteGenerator;
			
		return mod;
	};
}