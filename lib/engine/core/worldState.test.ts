
import assert from "node:assert/strict";
import { test } from "node:test";

import {
  createWorldState,
  advanceWorldState,
  serializeWorldState,
  deserializeWorldState,
} from "./worldState.ts";

import { recordWorldEvent } from "./eventLedger.ts";

test("Creates a new world", () => {
  const world = createWorldState("new-world", 12345);

  assert.equal(world.id, "new-world");
  assert.equal(world.seed, 12345);
  assert.equal(world.clock.currentDay, 0);
  assert.equal(world.ledger.events.length, 0);
});

test("Advances a world by ten years", () => {
  const world = createWorldState();
  const future = advanceWorldState(world, 3650);

  assert.equal(future.clock.currentDay, 3650);
  assert.equal(world.clock.currentDay, 0);
});

test("Saves and restores a world", () => {
  const world = advanceWorldState(
    createWorldState("world-001", 42),
    365
  );

  const saved = serializeWorldState(world);
  const restored = deserializeWorldState(saved);

  assert.deepEqual(restored, world);
});

test("Preserves historical events after restoration", () => {
  const world = createWorldState();

  const updated = {
    ...world,
    ledger: recordWorldEvent(world.ledger, {
      day: 42,
      type: "knowledge.discovered",
      actorId: "npc-004",
      targetId: null,
      data: { discovery: "irrigation" },
    }),
  };

  const restored = deserializeWorldState(
    serializeWorldState(updated)
  );

  assert.equal(restored.ledger.events.length, 1);
  assert.equal(restored.ledger.events[0].id, "evt-1");
  assert.equal(restored.ledger.events[0].day, 42);
  assert.equal(
    restored.ledger.events[0].data.discovery,
    "irrigation"
  );
});
