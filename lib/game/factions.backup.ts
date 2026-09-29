export type FactionType =
  | "guild"
  | "hunter"
  | "pirate"
  | "criminal"
  | "military"
  | "government"
  | "secret"
  | "religious"
  | "other";

export type FactionRelationship =
  | "ally"
  | "friendly"
  | "neutral"
  | "suspicious"
  | "hostile"
  | "enemy";

export interface Faction {
  id: string;
  name: string;
  type: FactionType;
  description: string;

  leader: string;

  members: string[];

  ranks: string[];

  territory: string[];

  goals: string[];

  allies: string[];
  enemies: string[];

  reputation: number;
  power: number;

  playerRelationship: FactionRelationship;

  active: boolean;
}

export const factions: Faction[] = [
  {
    id: "world-hunter-association",
    name: "World Hunter Association",
    type: "hunter",
    description:
      "A powerful international organization responsible for registering hunters, investigating supernatural threats, and coordinating operations against dangerous creatures.",
    leader: "Unknown",
    members: [],
    ranks: [
      "Novice Hunter",
      "Licensed Hunter",
      "Elite Hunter",
      "Master Hunter",
      "Grand Hunter",
    ],
    territory: ["Major Cities"],
    goals: [
      "Protect civilian populations",
      "Investigate supernatural threats",
      "Control dangerous zones",
      "Recruit powerful hunters",
    ],
    allies: [],
    enemies: [],
    reputation: 50,
    power: 80,
    playerRelationship: "neutral",
    active: true,
  },

  {
    id: "shadow-spiders",
    name: "The Shadow Spiders",
    type: "criminal",
    description:
      "A feared criminal organization whose members are bound together by loyalty, personal bonds, and a mysterious shared history.",
    leader: "The Spider King",
    members: [],
    ranks: [
      "Recruit",
      "Spider",
      "Elite Spider",
      "Inner Circle",
    ],
    territory: ["Unknown"],
    goals: [
      "Acquire forbidden artifacts",
      "Expand their influence",
      "Defeat powerful enemies",
      "Protect their members",
    ],
    allies: [],
    enemies: ["world-hunter-association"],
    reputation: -60,
    power: 90,
    playerRelationship: "suspicious",
    active: true,
  },

  {
    id: "dawn-cloaks",
    name: "The Dawn Cloaks",
    type: "secret",
    description:
      "A mysterious organization whose members manipulate events from behind the scenes while pursuing a goal known only to their highest-ranking members.",
    leader: "The First Cloak",
    members: [],
    ranks: [
      "Initiate",
      "Cloak",
      "Black Cloak",
      "High Cloak",
    ],
    territory: ["Unknown"],
    goals: [
      "Manipulate world events",
      "Recruit exceptional individuals",
      "Collect ancient knowledge",
    ],
    allies: [],
    enemies: [],
    reputation: 0,
    power: 95,
    playerRelationship: "neutral",
    active: true,
  },

  {
    id: "free-seas-pirates",
    name: "Free Seas Coalition",
    type: "pirate",
    description:
      "A loose collection of pirate crews who reject the authority of the world's governments and seek freedom, treasure, adventure, and legendary territories.",
    leader: "The Sea King",
    members: [],
    ranks: [
      "Deckhand",
      "Pirate",
      "Officer",
      "Captain",
      "Fleet Commander",
    ],
    territory: ["The Open Seas"],
    goals: [
      "Explore unknown waters",
      "Acquire legendary treasure",
      "Challenge powerful governments",
      "Build powerful pirate crews",
    ],
    allies: [],
    enemies: [],
    reputation: -30,
    power: 75,
    playerRelationship: "neutral",
    active: true,
  },
];

export function getFactionById(id: string): Faction | undefined {
  return factions.find((faction) => faction.id === id);
}

export function getFactionsByType(type: FactionType): Faction[] {
  return factions.filter((faction) => faction.type === type);
}
