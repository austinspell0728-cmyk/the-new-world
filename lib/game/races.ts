export interface Race {
  id: string;
  name: string;
  description: string;
  common: boolean;
  traits: string[];
  strengths: string[];
  weaknesses: string[];
  compatibleAncestries: string[];
}

export const races: Race[] = [
  {
    id: "human",
    name: "Human",
    description:
      "Adaptable and ambitious people found throughout the known world.",
    common: true,
    traits: ["Adaptable", "Ambitious", "Versatile"],
    strengths: ["Balanced potential", "Fast learning", "Social adaptability"],
    weaknesses: ["No extreme natural specialization"],
    compatibleAncestries: ["Human"],
  },

  {
    id: "elf",
    name: "Elf",
    description:
      "Long-lived beings naturally attuned to magic and the natural world.",
    common: true,
    traits: ["Long-lived", "Magically attuned", "Perceptive"],
    strengths: ["Magic", "Perception", "Dexterity"],
    weaknesses: ["Physically less durable"],
    compatibleAncestries: ["Elf", "Human"],
  },

  {
    id: "dwarf",
    name: "Dwarf",
    description:
      "Hardy people renowned for craftsmanship, endurance, and underground civilizations.",
    common: true,
    traits: ["Hardy", "Craftsman", "Resilient"],
    strengths: ["Endurance", "Strength", "Construction"],
    weaknesses: ["Lower natural agility"],
    compatibleAncestries: ["Dwarf", "Human"],
  },

  {
    id: "orc",
    name: "Orc",
    description:
      "Powerful warriors whose cultures value strength, survival, and honor.",
    common: true,
    traits: ["Powerful", "Resilient", "Warrior culture"],
    strengths: ["Strength", "Durability", "Physical combat"],
    weaknesses: ["Lower magical affinity"],
    compatibleAncestries: ["Orc", "Human"],
  },

  {
    id: "beastkin",
    name: "Beastkin",
    description:
      "Humanoid peoples possessing physical traits inherited from powerful animal lineages.",
    common: true,
    traits: ["Enhanced senses", "Natural instincts", "Variable physiology"],
    strengths: ["Speed", "Senses", "Physical specialization"],
    weaknesses: ["Traits vary heavily between lineages"],
    compatibleAncestries: ["Beastkin", "Human"],
  },

  {
    id: "dragonkin",
    name: "Dragonkin",
    description:
      "Extremely rare descendants of ancient draconic bloodlines.",
    common: false,
    traits: ["Draconic blood", "Enhanced durability", "Elemental affinity"],
    strengths: ["Physical potential", "Magic", "Elemental abilities"],
    weaknesses: ["Rare", "Often feared or hunted"],
    compatibleAncestries: ["Dragonkin", "Human", "Elf"],
  },

  {
    id: "fae",
    name: "Fae",
    description:
      "Mysterious beings connected to ancient magic and supernatural realms.",
    common: false,
    traits: ["Mystical", "Magical", "Otherworldly"],
    strengths: ["Magic", "Illusion", "Spirit abilities"],
    weaknesses: ["Physical fragility", "Vulnerable to certain supernatural forces"],
    compatibleAncestries: ["Fae", "Elf", "Human"],
  },
];

export function getRaceById(id: string): Race | undefined {
  return races.find((race) => race.id === id);
}

export function getCommonRaces(): Race[] {
  return races.filter((race) => race.common);
}

export function getRareRaces(): Race[] {
  return races.filter((race) => !race.common);
}
