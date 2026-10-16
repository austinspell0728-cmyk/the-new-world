
import assert from "node:assert/strict";
import { test } from "node:test";
import ts from "typescript";
import fs from "node:fs";
import vm from "node:vm";

const source = fs.readFileSync(
  "lib/engine/core/worldClock.ts",
  "utf8"
);

const compiled = ts.transpileModule(source, {
  compilerOptions: {
    module: ts.ModuleKind.CommonJS,
    target: ts.ScriptTarget.ES2020,
  },
}).outputText;

const exports = {};
vm.runInNewContext(compiled, { exports });

const {
  createWorldClock,
  advanceWorldClock,
  getWorldYear,
  getDayOfYear,
} = exports;

test("World Clock starts on Day 0", () => {
  const clock = createWorldClock();

  assert.equal(clock.currentDay, 0);
  assert.equal(getWorldYear(clock), 1);
  assert.equal(getDayOfYear(clock), 1);
});

test("World Clock advances 10 years", () => {
  const clock = createWorldClock();
  const future = advanceWorldClock(clock, 3650);

  assert.equal(future.currentDay, 3650);
  assert.equal(getWorldYear(future), 11);
  assert.equal(getDayOfYear(future), 1);
});

test("Advancing time does not modify the original clock", () => {
  const clock = createWorldClock();
  const future = advanceWorldClock(clock, 100);

  assert.equal(clock.currentDay, 0);
  assert.equal(future.currentDay, 100);
});

test("World Clock rejects negative time", () => {
  assert.throws(() => advanceWorldClock(createWorldClock(), -1));
});