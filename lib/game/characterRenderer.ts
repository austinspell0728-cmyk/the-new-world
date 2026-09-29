export interface CharacterAppearance {
  skinTone: string;
  hairColor: string;
  hairStyle: string;
  eyeColor: string;
  bodyType: string;
  height: string;
  clothing: string;
  accessories: string[];
  raceFeatures: string[];
}

export interface CharacterPortraitData {
  primaryRace: string;
  ancestry: string[];
  appearance: CharacterAppearance;
}

function pick<T>(items: T[]): T {
  return items[Math.floor(Math.random() * items.length)];
}

const skinTones = [
  "fair",
  "light",
  "tan",
  "brown",
  "dark brown",
  "deep",
];

const hairColors = [
  "black",
  "dark brown",
  "brown",
  "auburn",
  "blonde",
  "white",
  "silver",
  "red",
];

const hairStyles = [
  "short",
  "medium length",
  "long",
  "messy",
  "braided",
  "tied back",
];

const eyeColors = [
  "brown",
  "blue",
  "green",
  "gray",
  "amber",
  "gold",
];

const bodyTypes = [
  "slender",
  "lean",
  "athletic",
  "muscular",
  "stocky",
];

const heights = [
  "short",
  "average height",
  "tall",
  "very tall",
];

const clothing = [
  "simple adventurer clothing",
  "leather armor",
  "traveling clothes",
  "light armor",
  "heavy adventurer armor",
];

export function getRaceFeatures(race: string): string[] {
  const normalizedRace = race.toLowerCase();

  if (normalizedRace.includes("elf")) {
    return [
      "naturally pointed ears",
      "refined facial structure",
      "slightly elongated ears",
    ];
  }

  if (normalizedRace.includes("dwarf")) {
    return [
      "short stature",
      "broad shoulders",
      "thick build",
      "strong facial structure",
    ];
  }

  if (normalizedRace.includes("orc")) {
    return [
      "small tusks",
      "strong jaw",
      "prominent brow",
      "powerful facial structure",
    ];
  }

  if (
    normalizedRace.includes("dragon") ||
    normalizedRace.includes("dragonkin")
  ) {
    return [
      "small horns",
      "subtle scales",
      "slightly reptilian facial features",
    ];
  }

  if (
    normalizedRace.includes("beast") ||
    normalizedRace.includes("wolf") ||
    normalizedRace.includes("fox")
  ) {
    return [
      "animal ears",
      "slightly animalistic facial features",
    ];
  }

  return [];
}

export function getAncestryFeatures(ancestry: string[]): string[] {
  const features: string[] = [];

  for (const race of ancestry) {
    features.push(...getRaceFeatures(race));
  }

  return [...new Set(features)];
}

export function generateCharacterAppearance(
  character: CharacterPortraitData
): CharacterPortraitData {
  const raceFeatures = [
    ...getRaceFeatures(character.primaryRace),
    ...getAncestryFeatures(character.ancestry),
  ];

  return {
    ...character,

    appearance: {
      skinTone: pick(skinTones),
      hairColor: pick(hairColors),
      hairStyle: pick(hairStyles),
      eyeColor: pick(eyeColors),
      bodyType: pick(bodyTypes),
      height: pick(heights),
      clothing: pick(clothing),
      accessories: [],
      raceFeatures: [...new Set(raceFeatures)],
    },
  };
}

export function createCharacterPortraitPrompt(
  character: CharacterPortraitData
): string {
  const appearance = character.appearance;

  const ancestry =
    character.ancestry.length > 0
      ? character.ancestry.join(", ")
      : character.primaryRace;

  const features =
    appearance.raceFeatures.length > 0
      ? appearance.raceFeatures.join(", ")
      : "natural humanoid features";

  return [
    "Create a highly detailed realistic fantasy character portrait.",
    "The character should look like a believable living person rather than a cartoon.",
    "Photorealistic fantasy character concept art.",
    "Natural human anatomy and realistic proportions.",
    "Detailed skin texture.",
    "Detailed eyes and facial features.",
    "Realistic hair strands.",
    "Cinematic fantasy lighting.",
    "High quality facial detail.",

    `Primary race: ${character.primaryRace}.`,
    `Ancestry: ${ancestry}.`,

    `Skin tone: ${appearance.skinTone}.`,
    `Hair color: ${appearance.hairColor}.`,
    `Hair style: ${appearance.hairStyle}.`,
    `Eye color: ${appearance.eyeColor}.`,
    `Body type: ${appearance.bodyType}.`,
    `Height: ${appearance.height}.`,
    `Clothing: ${appearance.clothing}.`,

    `Race-specific physical characteristics: ${features}.`,

    appearance.accessories.length > 0
      ? `Accessories: ${appearance.accessories.join(", ")}.`
      : "No unnecessary accessories.",

    "The race characteristics should be clearly visible but naturally integrated into the character.",
    "Avoid exaggerated cartoon proportions.",
    "Avoid generic fantasy character design.",
    "Avoid plastic-looking skin.",
    "Avoid distorted anatomy.",
    "Avoid extra limbs or fingers.",
    "Portrait composition.",
    "Character facing the viewer.",
    "Neutral or subtle confident expression.",
    "Detailed fantasy RPG character artwork.",
  ].join(" ");
}

/*
 * Temporary fallback renderer.
 *
 * This keeps the game working while we connect
 * the real AI image-generation system.
 */
export function generateCharacterPortrait(
  appearance: CharacterAppearance
): string {
  const skin =
    appearance.skinTone.toLowerCase().includes("fair")
      ? "#f0c8b0"
      : appearance.skinTone.toLowerCase().includes("light")
        ? "#ddb092"
        : appearance.skinTone.toLowerCase().includes("tan")
          ? "#bd815d"
          : appearance.skinTone.toLowerCase().includes("dark")
            ? "#633d2d"
            : "#956044";

  const hair =
    appearance.hairColor === "blonde"
      ? "#d4ad58"
      : appearance.hairColor === "white"
        ? "#e8e8e8"
        : appearance.hairColor === "silver"
          ? "#aeb6c2"
          : appearance.hairColor === "red"
            ? "#8d3325"
            : "#302018";

  const eyes =
    appearance.eyeColor === "blue"
      ? "#4f91d1"
      : appearance.eyeColor === "green"
        ? "#4f9c62"
        : appearance.eyeColor === "gold"
          ? "#d5aa32"
          : appearance.eyeColor === "amber"
            ? "#b87927"
            : "#493126";

  const features = appearance.raceFeatures.join(" ").toLowerCase();

  const pointedEars = features.includes("pointed");
  const horns = features.includes("horn");
  const tusks = features.includes("tusk");
  const beard = features.includes("beard");

  return `
<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 500 620"
  width="500"
  height="620"
>
  <defs>
    <linearGradient id="background" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#263241"/>
      <stop offset="100%" stop-color="#09090b"/>
    </linearGradient>
  </defs>

  <rect width="500" height="620" rx="24" fill="url(#background)" />

  <path
    d="M75 620 Q85 430 170 385 Q250 350 330 385 Q415 430 425 620Z"
    fill="#424b56"
  />

  ${
    pointedEars
      ? `
        <path d="M135 190 L50 120 L105 235Z" fill="${skin}" />
        <path d="M365 190 L450 120 L395 235Z" fill="${skin}" />
      `
      : ""
  }

  <ellipse
    cx="250"
    cy="215"
    rx="125"
    ry="155"
    fill="${skin}"
  />

  ${
    horns
      ? `
        <path
          d="M145 100 Q100 45 115 20 Q155 50 165 105Z"
          fill="#44352b"
        />
        <path
          d="M355 100 Q400 45 385 20 Q345 50 335 105Z"
          fill="#44352b"
        />
      `
      : ""
  }

  <path
    d="
      M125 155
      Q125 65 250 50
      Q375 65 375 155
      Q330 105 250 120
      Q170 105 125 155
      Z
    "
    fill="${hair}"
  />

  <path
    d="M155 180 Q185 160 210 178"
    fill="none"
    stroke="${hair}"
    stroke-width="9"
  />

  <path
    d="M290 178 Q315 160 345 180"
    fill="none"
    stroke="${hair}"
    stroke-width="9"
  />

  <ellipse cx="185" cy="210" rx="29" ry="17" fill="#f4f4f4" />
  <ellipse cx="315" cy="210" rx="29" ry="17" fill="#f4f4f4" />

  <circle cx="185" cy="210" r="10" fill="${eyes}" />
  <circle cx="315" cy="210" r="10" fill="${eyes}" />

  <circle cx="185" cy="210" r="4" fill="#050505" />
  <circle cx="315" cy="210" r="4" fill="#050505" />

  <path
    d="M250 205 Q230 260 250 275 Q270 260 250 205"
    fill="none"
    stroke="#754a3b"
    stroke-width="5"
  />

  <path
    d="M205 305 Q250 325 295 305"
    fill="none"
    stroke="#542f2b"
    stroke-width="5"
  />

  ${
    tusks
      ? `
        <path d="M155 245 Q155 285 170 255" fill="#eee5c9" />
        <path d="M345 245 Q345 285 330 255" fill="#eee5c9" />
      `
      : ""
  }

  ${
    beard
      ? `
        <path
          d="M145 250 Q250 360 355 250 L330 345 Q250 390 170 345Z"
          fill="${hair}"
        />
      `
      : ""
  }

  <text
    x="250"
    y="575"
    text-anchor="middle"
    fill="white"
    fill-opacity="0.65"
    font-family="sans-serif"
    font-size="16"
    letter-spacing="3"
  >
    AI PORTRAIT READY
  </text>
</svg>
`;
}
