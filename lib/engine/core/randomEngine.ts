
export interface RandomState {
  state: number;
}

export function createRandomState(seed: number): RandomState {
  if (!Number.isSafeInteger(seed)) {
    throw new Error("Seed must be a safe integer.");
  }

  return { state: seed >>> 0 };
}

export function nextRandom(
  random: RandomState
): { value: number; random: RandomState } {
  // Mulberry32: a small deterministic PRNG.
  const state = (random.state + 0x6D2B79F5) >>> 0;

  let t = state;
  t = Math.imul(t ^ (t >>> 15), t | 1);
  t ^= t + Math.imul(t ^ (t >>> 7), t | 61);

  const value = ((t ^ (t >>> 14)) >>> 0) / 4294967296;

  return {
    value,
    random: { state },
  };
}

export function randomInt(
  random: RandomState,
  min: number,
  max: number
): { value: number; random: RandomState } {
  if (
    !Number.isSafeInteger(min) ||
    !Number.isSafeInteger(max) ||
    min > max ||
    max - min + 1 > 4294967296
  ) {
    throw new Error("Invalid random integer range.");
  }

  const result = nextRandom(random);

  return {
    value: min + Math.floor(result.value * (max - min + 1)),
    random: result.random,
  };
}
