
export interface WorldClock {
  currentDay: number;
  daysPerYear: number;
}

export function createWorldClock(
  startingDay = 0,
  daysPerYear = 365
): WorldClock {
  if (!Number.isSafeInteger(startingDay) || startingDay < 0) {
    throw new Error("Starting day must be a nonnegative safe integer.");
  }

  if (!Number.isSafeInteger(daysPerYear) || daysPerYear <= 0) {
    throw new Error("Days per year must be a positive safe integer.");
  }

  return {
    currentDay: startingDay,
    daysPerYear,
  };
}

export function advanceWorldClock(
  clock: WorldClock,
  days = 1
): WorldClock {
  if (!Number.isSafeInteger(days) || days < 0) {
    throw new Error("Days must be a nonnegative safe integer.");
  }

  const nextDay = clock.currentDay + days;

  if (!Number.isSafeInteger(nextDay)) {
    throw new Error("World clock exceeded its safe range.");
  }

  return {
    ...clock,
    currentDay: nextDay,
  };
}

export function getWorldYear(clock: WorldClock): number {
  return Math.floor(clock.currentDay / clock.daysPerYear) + 1;
}

export function getDayOfYear(clock: WorldClock): number {
  return (clock.currentDay % clock.daysPerYear) + 1;
}
