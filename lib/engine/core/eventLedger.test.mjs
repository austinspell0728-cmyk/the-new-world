
import assert from "node:assert/strict";
import { test } from "node:test";
import fs from "node:fs";
import ts from "typescript";
import vm from "node:vm";

const source = fs.readFileSync(
  "lib/engine/core/eventLedger.ts",
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
  createEventLedger,
  recordWorldEvent,
  getEventsByType,
  getEventsByActor,
  getEventsThroughDay,
} = exports;

function makeEvent(day, type, actorId = "npc-004") {
  return {
    day,
    type,
    actorId,
    targetId: null,
    data: {},
  };
}

test("Creates an empty event ledger", () => {
  const ledger = createEventLedger();

  assert.equal(ledger.events.length, 0);
  assert.equal(ledger.nextEventId, 1);
});

test("Records events with unique IDs", () => {
  let ledger = createEventLedger();

  ledger = recordWorldEvent(
    ledger,
    makeEvent(42, "knowledge.discovered")
  );

  ledger = recordWorldEvent(
    ledger,
    makeEvent(63, "knowledge.taught")
  );

  assert.equal(ledger.events.length, 2);
  assert.equal(ledger.events[0].id, "evt-1");
  assert.equal(ledger.events[1].id, "evt-2");
});

test("Rejects events recorded out of order", () => {
  const ledger = recordWorldEvent(
    createEventLedger(),
    makeEvent(100, "npc.born")
  );

  assert.throws(() =>
    recordWorldEvent(
      ledger,
      makeEvent(50, "npc.died")
    )
  );
});

test("Finds events by type and actor", () => {
  let ledger = createEventLedger();

  ledger = recordWorldEvent(
    ledger,
    makeEvent(10, "knowledge.discovered", "npc-004")
  );

  ledger = recordWorldEvent(
    ledger,
    makeEvent(20, "npc.died", "npc-007")
  );

  assert.equal(
    getEventsByType(ledger, "npc.died").length,
    1
  );

  assert.equal(
    getEventsByActor(ledger, "npc-004").length,
    1
  );
});

test("Finds events through a specified day", () => {
  let ledger = createEventLedger();

  ledger = recordWorldEvent(
    ledger,
    makeEvent(10, "npc.born")
  );

  ledger = recordWorldEvent(
    ledger,
    makeEvent(30, "knowledge.discovered")
  );

  assert.equal(getEventsThroughDay(ledger, 15).length, 1);
  assert.equal(getEventsThroughDay(ledger, 30).length, 2);
});

test("Does not modify the original ledger", () => {
  const original = createEventLedger();

  const updated = recordWorldEvent(
    original,
    makeEvent(5, "npc.born")
  );

  assert.equal(original.events.length, 0);
  assert.equal(updated.events.length, 1);
});
