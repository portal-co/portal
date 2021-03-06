// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --allow-natives-syntax --turbofan

/* Test deopt behaviors when the prototype has elements */

// reduce

(function () {
  var array = [, 3];

  function accumulate (prev, cur, curIdx, arr) { arr[curIdx] = cur + prev; }
  function reduce() {
    array.reduce(accumulate);
  }

  %PrepareFunctionForOptimization(reduce);
  reduce(); reduce();

  %OptimizeFunctionOnNextCall(reduce);

  reduce();
  assertEquals(array, [,3]);

  array.__proto__.push(3);
  // deopt
  reduce();
  assertEquals(array, [, 6]);
  assertEquals(Object.getOwnPropertyDescriptor(array, 0), undefined);
})();
