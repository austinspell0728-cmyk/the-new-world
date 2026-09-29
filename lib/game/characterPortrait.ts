export interface PortraitInput {
  skinTone: string;
  hairColor: string;
  hairStyle: string;
  eyeColor: string;
  bodyType: string;
  clothing: string;
  raceFeatures: string[];
}

function findMatch(value: string, words: string[]): boolean {
  const lower = value.toLowerCase();

  return words.some((word) => lower.includes(word));
}

function getSkinColor(tone: string): string {
  const value = tone.toLowerCase();

  if (value.includes("fair")) return "#f3d2bf";
  if (value.includes("light")) return "#e7b99d";
  if (value.includes("tan")) return "#c58c68";
  if (value.includes("brown")) return "#875437";
  if (value.includes("dark")) return "#5c3829";
  if (value.includes("pale")) return "#ead8cf";

  return "#c58c68";
}

function getHairColor(color: string): string {
  const value = color.toLowerCase();

  if (value.includes("black")) return "#171717";
  if (value.includes("brown")) return "#4b2d1c";
  if (value.includes("blonde")) return "#d5b45c";
  if (value.includes("white")) return "#eeeeee";
  if (value.includes("silver")) return "#b8bec8";
  if (value.includes("red")) return "#963b29";
  if (value.includes("auburn")) return "#7d3926";

  return "#171717";
}

function getEyeColor(color: string): string {
  const value = color.toLowerCase();

  if (value.includes("blue")) return "#4b8dcc";
  if (value.includes("green")) return "#4d9a61";
  if (value.includes("gray")) return "#8c969e";
  if (value.includes("grey")) return "#8c969e";
  if (value.includes("gold")) return "#e0b83f";
  if (value.includes("amber")) return "#c78b32";
  if (value.includes("red")) return "#b23b35";

  return "#4a2c20";
}

function getClothingColor(clothing: string): string {
  const value = clothing.toLowerCase();

  if (value.includes("armor")) return "#414955";
  if (value.includes("leather")) return "#704b30";
  if (value.includes("robe")) return "#544866";
  if (value.includes("cloth")) return "#59636b";
  if (value.includes("travel")) return "#665541";

  return "#454545";
}

function getBodyScale(bodyType: string): {
  shoulder: number;
  torso: number;
} {
  const value = bodyType.toLowerCase();

  if (
    findMatch(value, ["large", "muscular", "heavy", "broad"])
  ) {
    return {
      shoulder: 150,
      torso: 105,
    };
  }

  if (
    findMatch(value, ["slender", "thin", "lean"])
  ) {
    return {
      shoulder: 105,
      torso: 82,
    };
  }

  return {
    shoulder: 125,
    torso: 92,
  };
}

export function generateCharacterPortrait(
  input: PortraitInput
): string {
  const skin = getSkinColor(input.skinTone);
  const hair = getHairColor(input.hairColor);
  const eyes = getEyeColor(input.eyeColor);
  const clothing = getClothingColor(input.clothing);

  const features = input.raceFeatures;

const pointedEars = findMatch(features.join(" "), [
  "pointed ear",
  "pointed ears",
  "elf ear",
  "elf ears",
  "elven ear",
  "elven ears",
  "sharp ear",
  "sharp ears",
]);

  const horns = findMatch(features.join(" "), [
    "horn",
    "antler",
  ]);

  const tusks = findMatch(features.join(" "), [
    "tusk",
    "fang",
  ]);

  const scales = findMatch(features.join(" "), [
    "scale",
    "scaled",
  ]);

  const beard = findMatch(features.join(" "), [
    "beard",
    "facial hair",
  ]);

  const markings = findMatch(features.join(" "), [
    "marking",
    "tattoo",
    "rune",
  ]);

  const body = getBodyScale(input.bodyType);

const ears = pointedEars
  ? `
      <path
        d="M135 185 L70 145 L125 225 Z"
        fill="${skin}"
        stroke="#171717"
        stroke-width="4"
      />

      <path
        d="M365 185 L430 145 L375 225 Z"
        fill="${skin}"
        stroke="#171717"
        stroke-width="4"
      />

      <path
        d="M125 180 L92 158 L120 205"
        fill="none"
        stroke="#a36d5b"
        stroke-width="3"
      />

      <path
        d="M375 180 L408 158 L380 205"
        fill="none"
        stroke="#a36d5b"
        stroke-width="3"
      />
    `
    : `
<ellipse
  cx="122"
  cy="185"
  rx="23"
  ry="35"
  fill="${skin}"
  stroke="#171717"
  stroke-width="4"
/>

<ellipse
  cx="378"
  cy="185"
  rx="23"
  ry="35"
  fill="${skin}"
  stroke="#171717"
  stroke-width="4"
/>
    `;

  const hornLayer = horns
    ? `
      <path
        d="M130 105
           Q88 55 103 25
           Q139 52 151 105 Z"
        fill="#40352d"
        stroke="#171717"
        stroke-width="4"
      />

      <path
        d="M270 105
           Q312 55 297 25
           Q261 52 249 105 Z"
        fill="#40352d"
        stroke="#171717"
        stroke-width="4"
      />

      <path
        d="M111 55 L125 69"
        stroke="#796657"
        stroke-width="4"
      />

      <path
        d="M289 55 L275 69"
        stroke="#796657"
        stroke-width="4"
      />
    `
    : "";

  const hairLayer =
    input.hairStyle.toLowerCase().includes("long")
      ? `
        <path
          d="
            M94 153
            Q82 62 200 48
            Q318 62 306 153
            L292 270
            L258 220
            L142 220
            L108 270 Z
          "
          fill="${hair}"
        />
      `
      : input.hairStyle.toLowerCase().includes("braid")
      ? `
        <path
          d="
            M94 150
            Q86 65 200 48
            Q314 65 306 150
            Q270 103 200 92
            Q130 103 94 150
          "
          fill="${hair}"
        />

        <path
          d="
            M103 160
            Q82 195 100 230
            Q82 260 105 290
          "
          fill="none"
          stroke="${hair}"
          stroke-width="24"
          stroke-linecap="round"
        />

        <path
          d="
            M297 160
            Q318 195 300 230
            Q318 260 295 290
          "
          fill="none"
          stroke="${hair}"
          stroke-width="24"
          stroke-linecap="round"
        />
      `
      : `
        <path
          d="
            M94 151
            Q82 65 200 48
            Q318 65 306 151
            Q278 112 241 119
            Q201 126 163 112
            Q126 106 94 151 Z
          "
          fill="${hair}"
        />
      `;

  const beardLayer = beard
    ? `
      <path
        d="
          M137 235
          Q200 295 263 235
          L247 285
          Q200 320 153 285 Z
        "
        fill="${hair}"
      />

      <path
        d="
          M151 260
          Q200 286 249 260
        "
        fill="none"
        stroke="#242424"
        stroke-width="3"
      />
    `
    : "";

  const tuskLayer = tusks
    ? `
      <path
        d="M151 241 Q151 279 165 255"
        fill="#f0e6c8"
        stroke="#222"
        stroke-width="3"
      />

      <path
        d="M249 241 Q249 279 235 255"
        fill="#f0e6c8"
        stroke="#222"
        stroke-width="3"
      />
    `
    : "";

  const scaleLayer = scales
    ? `
      <g fill="#78988e" opacity="0.9">
        <path d="M125 207 Q136 195 147 207 Q136 219 125 207" />
        <path d="M147 218 Q158 206 169 218 Q158 230 147 218" />
        <path d="M231 218 Q242 206 253 218 Q242 230 231 218" />
        <path d="M253 207 Q264 195 275 207 Q264 219 253 207" />
        <path d="M181 270 Q200 254 219 270 Q200 286 181 270" />
      </g>
    `
    : "";

  const markingLayer = markings
    ? `
      <g
        fill="none"
        stroke="#9d65d4"
        stroke-width="3"
        opacity="0.8"
      >
        <path d="M126 210 Q145 230 126 250" />
        <path d="M274 210 Q255 230 274 250" />
        <path d="M188 120 Q200 105 212 120" />
      </g>
    `
    : "";

  return `
<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 500 620"
  width="500"
  height="620"
>
  <defs>
    <linearGradient
      id="background"
      x1="0"
      y1="0"
      x2="0"
      y2="1"
    >
      <stop
        offset="0%"
        stop-color="#1f2937"
      />

      <stop
        offset="100%"
        stop-color="#09090b"
      />
    </linearGradient>

    <radialGradient
      id="faceLight"
      cx="50%"
      cy="35%"
      r="65%"
    >
      <stop
        offset="0%"
        stop-color="#ffffff"
        stop-opacity="0.08"
      />

      <stop
        offset="100%"
        stop-color="#000000"
        stop-opacity="0.15"
      />
    </radialGradient>
  </defs>

  <!-- Background -->

  <rect
    width="500"
    height="620"
    rx="24"
    fill="url(#background)"
  />

  <!-- Portrait frame -->

  <rect
    x="12"
    y="12"
    width="476"
    height="596"
    rx="20"
    fill="none"
    stroke="#ffffff"
    stroke-opacity="0.12"
    stroke-width="2"
  />

  <!-- Shoulders -->

  <path
    d="
      M${250 - body.shoulder}
      620

      Q
      ${250 - body.shoulder + 10}
      420

      ${250 - body.torso}
      360

      Q
      250
      325

      ${250 + body.torso}
      360

      Q
      ${250 + body.shoulder - 10}
      420

      ${250 + body.shoulder}
      620

      Z
    "
    fill="${clothing}"
    stroke="#171717"
    stroke-width="5"
  />

  <!-- Clothing collar -->

  <path
    d="
      M155 365
      Q200 410 250 425
      Q300 410 345 365
    "
    fill="none"
    stroke="#929292"
    stroke-width="5"
  />

  <!-- Neck -->

  <path
    d="
      M185 280
      L185 370
      Q250 405 315 370
      L315 280 Z
    "
    fill="${skin}"
    stroke="#171717"
    stroke-width="4"
  />


  <!-- Head -->

  <ellipse
    cx="250"
    cy="210"
    rx="125"
    ry="155"
    fill="${skin}"
    stroke="#171717"
    stroke-width="5"
  />

  <!-- Face lighting -->

  <ellipse
    cx="250"
    cy="210"
    rx="120"
    ry="150"
    fill="url(#faceLight)"
  />

   <!-- Ears -->
${ears}

  <!-- Horns -->

  ${hornLayer}

  <!-- Hair -->

  ${hairLayer}

  <!-- Eyebrows -->

  <path
    d="M158 178 Q185 160 208 178"
    fill="none"
    stroke="${hair}"
    stroke-width="9"
    stroke-linecap="round"
  />

  <path
    d="M292 178 Q315 160 342 178"
    fill="none"
    stroke="${hair}"
    stroke-width="9"
    stroke-linecap="round"
  />

  <!-- Eyes -->

  <ellipse
    cx="183"
    cy="205"
    rx="28"
    ry="17"
    fill="#f4f4f4"
  />

  <ellipse
    cx="317"
    cy="205"
    rx="28"
    ry="17"
    fill="#f4f4f4"
  />

  <ellipse
    cx="183"
    cy="205"
    rx="10"
    ry="13"
    fill="${eyes}"
  />

  <ellipse
    cx="317"
    cy="205"
    rx="10"
    ry="13"
    fill="${eyes}"
  />

  <circle
    cx="183"
    cy="205"
    r="4"
    fill="#050505"
  />

  <circle
    cx="317"
    cy="205"
    r="4"
    fill="#050505"
  />

  <!-- Nose -->

  <path
    d="
      M250 205
      Q228 265 250 275
      Q272 265 250 205
    "
    fill="none"
    stroke="#754a3b"
    stroke-width="5"
    stroke-linecap="round"
  />

  <!-- Nose shadow -->

  <path
    d="M237 273 Q250 280 263 273"
    fill="none"
    stroke="#6b4336"
    stroke-width="3"
  />

  <!-- Mouth -->

  <path
    d="
      M207 303
      Q250 325 293 303
    "
    fill="none"
    stroke="#542f2b"
    stroke-width="5"
    stroke-linecap="round"
  />

  <!-- Facial markings -->

  ${markingLayer}

  <!-- Scales -->

  ${scaleLayer}

  <!-- Tusks -->

  ${tuskLayer}

  <!-- Beard -->

  ${beardLayer}

  <!-- Clothing details -->

  <path
    d="
      M115 470
      L385 470
    "
    stroke="#ffffff"
    stroke-opacity="0.12"
    stroke-width="4"
  />

  <path
    d="
      M180 410
      L250 460
      L320 410
    "
    fill="none"
    stroke="#ffffff"
    stroke-opacity="0.16"
    stroke-width="5"
  />

  <!-- Portrait label -->

  <text
    x="250"
    y="575"
    text-anchor="middle"
    fill="#ffffff"
    fill-opacity="0.65"
    font-family="sans-serif"
    font-size="17"
    letter-spacing="3"
  >
    GENERATED CHARACTER
  </text>
</svg>
`;
}
