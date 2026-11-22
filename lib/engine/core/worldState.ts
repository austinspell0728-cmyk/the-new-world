import {
  createWorldClock,
  advanceWorldClock,
  type WorldClock,
} from "./worldClock.ts";

import {
  createEventLedger,
  type EventLedger,
} from "./eventLedger.ts";

export interface WorldState {
  id: string;
  seed: number;
  clock: WorldClock;
  ledger: EventLedger;
}

export function createWorldState(
  id = "world-001",
  seed = 1
): WorldState {
  if (!id.trim()) {
    throw new Error("World ID cannot be empty.");
  }

  if (!Number.isSafeInteger(seed)) {
    throw new Error("World seed must be a safe integer.");
  }

  return {
    id,
    seed,
    clock: createWorldClock(),
    ledger: createEventLedger(),
  };
}

export function advanceWorldState(
  world: WorldState,
  days = 1
): WorldState {
  return {
    ...world,
    clock: advanceWorldClock(world.clock, days),
  };
}

export function serializeWorldState(
  world: WorldState
): string {
  return JSON.stringify(world);
}

export function deserializeWorldState(
  serialized: string
): WorldState {
  const parsed = JSON.parse(serialized) as WorldState;

  if (
    typeof parsed.id !== "string" ||
    !Number.isSafeInteger(parsed.seed) ||
    !parsed.clock ||
    !parsed.ledger
  ) {
    throw new Error("Invalid serialized world state.");
  }

  return parsed;
}