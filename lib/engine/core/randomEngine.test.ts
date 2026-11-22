
import assert from "node:assert/strict";
import { test } from "node:test";

import {
  createRandomState,
  nextRandom,
  randomInt,
} from "./randomEngine.ts";

test("Identical seeds produce identical sequences", () => {
  let a = createRandomState(12345);
  let b = createRandomState(12345);

  for (let i = 0; i < 1000; i++) {
    const nextA = nextRandom(a);
    const nextB = nextRandom(b);

    assert.equal(nextA.value, nextB.value);
    a = nextA.random;
    b = nextB.random;
  }
});

test("Different seeds produce different sequences", () => {
  const a = nextRandom(createRandomState(12345));
  const b = nextRandom(createRandomState(54321));

  assert.notEqual(a.value, b.value);
});

test("Random values stay between zero and one", () => {
  let random = createRandomState(42);

  for (let i = 0; i < 1000; i++) {
    const result = nextRandom(random);

    assert.ok(result.value >= 0);
    assert.ok(result.value < 1);

    random = result.random;
  }
});

test("Random integers remain within their range", () => {
  let random = createRandomState(99);

  for (let i = 0; i < 1000; i++) {
    const result = randomInt(random, 1, 20);

    assert.ok(result.value >= 1);
    assert.ok(result.value <= 20);
    assert.ok(Number.isInteger(result.value));

    random = result.random;
  }
});

test("Random generation preserves previous state", () => {
  const original = createRandomState(123);
  const result = nextRandom(original);

  assert.equal(original.state, 123);
  assert.notEqual(result.random.state, original.state);
});

test("Invalid inputs are rejected", () => {
  assert.throws(() => createRandomState(NaN));
  assert.throws(() => createRandomState(1.5));
  assert.throws(() => randomInt(createRandomState(1), 10, 1));
});
