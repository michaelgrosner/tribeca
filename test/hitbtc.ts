/// <reference path="../typings/mocha/mocha.d.ts" />

var assert = require("assert");

describe('HitBtc', () => {
  describe('positions()', function(){
    it('should work', function(){
      assert.equal(-1, [1,2,3].indexOf(5));
      assert.equal(-1, [1,2,3].indexOf(0));
    })
  })
});