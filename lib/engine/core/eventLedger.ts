
export type WorldEventType =
  | "npc.born"
  | "npc.died"
  | "knowledge.discovered"
  | "knowledge.learned"
  | "knowledge.taught"
  | "settlement.founded"
  | "resource.changed";

export interface WorldEvent {
  id: string;
  day: number;
  type: WorldEventType;
  actorId: string | null;
  targetId: string | null;
  data: Record<string, string | number | boolean | null>;
}

export interface EventLedger {
  events: WorldEvent[];
  nextEventId: number;
}

export function createEventLedger(): EventLedger {
  return {
    events: [],
    nextEventId: 1,
  };
}

export function recordWorldEvent(
  ledger: EventLedger,
  event: Omit<WorldEvent, "id">
): EventLedger {
  if (!Number.isSafeInteger(event.day) || event.day < 0) {
    throw new Error("Event day must be a nonnegative safe integer.");
  }

  if (
    ledger.events.length > 0 &&
    event.day < ledger.events[ledger.events.length - 1].day
  ) {
    throw new Error("Cannot record an event before the latest event.");
  }

  if (!Number.isSafeInteger(ledger.nextEventId) ||
      ledger.nextEventId < 1) {
    throw new Error("Invalid event sequence.");
  }

  const recordedEvent: WorldEvent = {
    ...event,
    data: { ...event.data },
    id: `evt-${ledger.nextEventId}`,
  };

  return {
    events: [...ledger.events, recordedEvent],
    nextEventId: ledger.nextEventId + 1,
  };
}

export function getEventsByType(
  ledger: EventLedger,
  type: WorldEventType
): WorldEvent[] {
  return ledger.events.filter((event) => event.type === type);
}

export function getEventsByActor(
  ledger: EventLedger,
  actorId: string
): WorldEvent[] {
  return ledger.events.filter(
    (event) => event.actorId === actorId
  );
}

export function getEventsThroughDay(
  ledger: EventLedger,
  day: number
): WorldEvent[] {
  return ledger.events.filter((event) => event.day <= day);
}
