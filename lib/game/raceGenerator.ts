import { races, Race } from "./races";

export interface GeneratedRace {
  primaryRace: Race;
  ancestry: Race[];
  traits: string[];
  strengths: string[];
  weaknesses: string[];
  potential: number;
}

export function generateRace(): GeneratedRace {
  const roll = Math.random();

  let ancestryCount = 1;

  if (roll < 0.001) {
    ancestryCount = 4;
  } else if (roll < 0.01) {
    ancestryCount = 3;
  } else if (roll < 0.08) {
    ancestryCount = 2;
  }

  const selected: Race[] = [];

  while (selected.length < ancestryCount) {
    const race = races[Math.floor(Math.random() * races.length)];

    if (!selected.some((existing) => existing.id === race.id)) {
      selected.push(race);
    }
  }

  const primaryRace = selected[0];

  const traits = [
    ...new Set(selected.flatMap((race) => race.traits)),
  ];

  const strengths = [
    ...new Set(selected.flatMap((race) => race.strengths)),
  ];

  const weaknesses = [
    ...new Set(selected.flatMap((race) => race.weaknesses)),
  ];

  let potential = 50;

  potential += (selected.length - 1) * 15;

  if (selected.some((race) => !race.common)) {
    potential += 10;
  }

  potential = Math.min(potential, 100);

  return {
    primaryRace,
    ancestry: selected,
    traits,
    strengths,
    weaknesses,
    potential,
  };
}
