var assert = require("assert");
var util = require("util");
var SortedArray = require("collections/sorted-array");
var _ = require("lodash");
var request = require("request");

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

            var eq = function(a, b) {
                return a.price === b.price;
            };

            var cmp  = function(a, b) {
                if (a.price === b.price) return 0;
                return a.price > b.price ? 1 : -1
            };

            var lastBids = new SortedArray([], eq, cmp);

            for (var i = 0; i < incoming.length; i++) {
                lastBids.push(incoming[i]);
            }

            console.log("before", util.inspect(lastBids.array, true, 500, true));

            lastBids.delete({price: 239.67, size: 0});

            console.log("delete", util.inspect(lastBids.array, true, 500, true));

            lastBids.add(incoming[2]);

            console.log("final", util.inspect(lastBids.array, true, 500, true));

            console.log(lastBids.slice(0, 3));
            assert(lastBids.has(incoming[0]));
        })
    });
});

describe("Winkdex", function() {
    it("should work", function(done) {
        request.get(
            {url: "https://winkdex.com/api/v0/price"},
            function (err, body, res) {
                if (err) {
                    this._log("error trying to get winkdex value %s", err);
                }
                else {
                    try {
                        var price = JSON.parse(res).price / 100.0;
                        console.log(price);
                        done();
                    }
                    catch (e) {
                        this._log("error handling winkdex value %s %j %j", e, body, res);
                    }
                }
            });
    });
})