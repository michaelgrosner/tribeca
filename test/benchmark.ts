import assert = require("assert");
import Utils = require("../src/server/utils");
import Statistics = require("../src/server/statistics");
var bindings = require('bindings')('ctubio.node');
var Benchmark = require('benchmark');

// function computeStdev(sequence: number[], factor: number, minTick: number): number {
  // const n = sequence.length;
  // let sum = 0;
  // sequence.forEach((x) => sum += x);
  // const mean = sum / n;
  // let variance = 0.0;
  // let v1 = 0.0;
  // let v2 = 0.0;
  // if (n != 1) {
      // for (let i = 0; i<n; i++) {
          // v1 = v1 + (sequence[i] - mean) * (sequence[i] - mean);
          // v2 = v2 + (sequence[i] - mean);
      // }
      // v2 = v2 * v2 / n;
      // variance = (v1 - v2) / (n-1);
      // if (variance < 0) variance = 0;
  // }
  // return Utils.roundNearest(Math.sqrt(variance) * factor, minTick);
// }

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
