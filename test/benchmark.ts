import assert = require("assert");
import Utils = require("../src/server/utils");
import Statistics = require("../src/server/statistics");
var bindings = require('bindings')('ctubio.node');
var Benchmark = require('benchmark');

describe("Benchmark C shared objects", () => {
    it("JS round is faster", () => {
      // var val = 1.23456;
      // var min = 0.001;
      // var side = 1;
      // return (new Benchmark.Suite())
        // .add('JS Round', function() {
          // Utils.roundSide(val, min, side);
        // })
        // .add('C Round', function() {
          // bindings.roundSide(val, min, side);
        // })
        // .on('cycle', function(event) {
          // console.log('      '+String(event.target));
        // })
        // .on('complete', function complete(a,b) {
          // console.log('        Fastest: '+this.filter('fastest').map('name')[0]);
          // assert.equal(this.filter('fastest').map('name')[0], 'JS Round');
        // })
        // .run({ 'async': false });
    });

    it("C stdev is faster", () => {
      // var seqA = [];
      // var seqB = [];
      // var seqC = [];
      // var seqD = [];
      // for (var i = 0; i<36000;i++) {
        // seqA.push(Math.random());
        // seqB.push(Math.random());
        // seqC.push(Math.random());
        // seqD.push(Math.random());
      // }
      // var min = 0.001;
      // var mul = 1;
      // return (new Benchmark.Suite())
        // .add('JS Stdev', function() {
          // Statistics.computeStdev(seqA, mul, min);
          // Statistics.computeStdev(seqB, mul, min);
          // Statistics.computeStdev(seqC, mul, min);
          // Statistics.computeStdev(seqD, mul, min);
        // })
        // .add('C Stdev', function() {
          // bindings.computeStdevs(
            // new Float64Array(seqA),
            // new Float64Array(seqB),
            // new Float64Array(seqC),
            // new Float64Array(seqD),
            // mul,
            // min
          // );
        // })
        // .on('cycle', function cycle(event) {
          // console.log('      '+String(event.target));
        // })
        // .on('complete', function complete (a,b) {
          // console.log('        Fastest: '+this.filter('fastest').map('name')[0]);
          // assert.equal(this.filter('fastest').map('name')[0], 'C Stdev');
        // })
        // .run({ 'async': false });
    });
});
