var assert = require("assert");
var util = require("util");
var SortedArray = require("collections/sorted-array");
var _ = require("lodash");


describe('HitBtc', function () {
    describe('map', function () {
        it('should work', function () {

            var incoming = [
                { price: 239.31, size: 17 },
                { price: 239.99, size: 33 },
                { price: 239.67, size: 1 },
                { price: 239.75, size: 44 },
                { price: 238.56, size: 25 }
            ];

            var cmp  = function(a, b) {
                if (a.price === b.price) return 0;
                return a.price > b.price ? 1 : -1
            };

            var lastBids = new SortedArray([], null, cmp);

            for (var i = 0; i < incoming.length; i++) {
                lastBids.push(incoming[i]);
            }

            console.log(util.inspect(lastBids, true, 500, true));

            assert(lastBids.has(incoming[0]));
        })
    });
});