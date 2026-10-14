"use client";

import { useEffect, useState } from "react";

import { supabase } from "../lib/supabase";
import { factions } from "../lib/game/factions";
import {
  getRaceFeatures,
  getAncestryFeatures,
  generateCharacterAppearance,
  createCharacterPortraitPrompt,
  CharacterPortraitData,
} from "../lib/game/characterRenderer";
import { generateCharacterPortrait } from "../lib/game/characterPortrait";
import { races as localRaces } from "../lib/game/races";
import { generateRace } from "../lib/game/raceGenerator";
export default function Home() {
const [testRace, setTestRace] =
  useState<ReturnType<typeof generateRace> | null>(null);

const [characterName, setCharacterName] = useState("");
const [selectedRace, setSelectedRace] = useState("");

const [hybridRaceOne, setHybridRaceOne] = useState("");
const [hybridRaceTwo, setHybridRaceTwo] = useState("");

const [skinTone, setSkinTone] = useState("Fair");
const [hairColor, setHairColor] = useState("Black");
const [hairStyle, setHairStyle] = useState("Short");
const [eyeColor, setEyeColor] = useState("Brown");
const [bodyType, setBodyType] = useState("Athletic");
const [height, setHeight] = useState("Average Height");

const [characterCreated, setCharacterCreated] = useState(false);

  const [action, setAction] = useState("");
const [selectedFaction, setSelectedFaction] = useState<string | null>(null);
  const [races, setRaces] = useState<string[]>([]);
const [characterAppearance, setCharacterAppearance] =
  useState<CharacterPortraitData | null>(null);
  const [characterPortrait, setCharacterPortrait] =
  useState<string | null>(null);  
  const [error, setError] = useState("");

  useEffect(() => {
    setRaces(localRaces.map((race) => race.name));
  }, []);
  useEffect(() => {
  if (!testRace) return;

if (!characterAppearance) {
  return;
}

setCharacterAppearance(characterAppearance);

setCharacterPortrait(
  generateCharacterPortrait(characterAppearance.appearance)
);

console.log(
  "Character portrait prompt:",
  createCharacterPortraitPrompt(characterAppearance)
);

}, [testRace]);
  return (
    <main className="min-h-screen bg-black text-white">
      <header className="border-b border-white/10 px-6 py-4">
        <h1 className="text-2xl font-bold">
          The New World
        </h1>

        <p className="text-sm text-gray-400">
          An AI-driven fantasy world
        </p>
      </header>

      <div className="mx-auto grid max-w-7xl gap-6 p-6 lg:grid-cols-[1fr_300px]">

        <section className="min-h-[600px] rounded-xl border border-white/10 bg-zinc-950 p-6">

          <div className="mb-8">
            <p className="mb-3 text-sm uppercase tracking-widest text-gray-500">
              Current Location
            </p>

            <h2 className="text-3xl font-semibold">
              Character Creation
            </h2>
           </div>
           <div className="mt-6 max-w-md">
  <label className="mb-2 block text-sm text-gray-400">
    Character Name
  </label>

  <input
    value={characterName}
    onChange={(e) => setCharacterName(e.target.value)}
    placeholder="Enter your character's name..."
    className="w-full rounded-lg border border-white/10 bg-zinc-900 px-4 py-3 text-white outline-none focus:border-white/30"
  />
</div>
<div className="mt-6 max-w-md">
  <label className="mb-2 block text-sm text-gray-400">
    Primary Race
  </label>

  <select
    value={selectedRace}
    onChange={(e) => setSelectedRace(e.target.value)}
    className="w-full rounded-lg border border-white/10 bg-zinc-900 px-4 py-3 text-white outline-none focus:border-white/30"
  >
    <option value="">Choose a race...</option>

    {races.map((race) => (
      <option key={race} value={race}>
        {race}
      </option>
    ))}

    <option value="hybrid">
      Hybrid / Mixed Ancestry
    </option>
  </select>
</div>

{selectedRace === "hybrid" && (
  <div className="mt-6 rounded-lg border border-white/10 bg-black/40 p-5">

    <h3 className="mb-2 font-semibold text-white">
      Hybrid Ancestry
    </h3>

    <p className="mb-5 text-sm text-gray-500">
      Choose the two races that make up your character's hybrid ancestry.
    </p>

    <div className="space-y-4">

      <div>
        <label className="mb-2 block text-sm text-gray-400">
          First Ancestry
        </label>

        <select
          value={hybridRaceOne}
          onChange={(e) => setHybridRaceOne(e.target.value)}
          className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
        >
          <option value="">Choose a race...</option>

          {races.map((race) => (
            <option key={race} value={race}>
              {race}
            </option>
          ))}
        </select>
      </div>

      <div>
        <label className="mb-2 block text-sm text-gray-400">
          Second Ancestry
        </label>

        <select
          value={hybridRaceTwo}
          onChange={(e) => setHybridRaceTwo(e.target.value)}
          className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
        >
          <option value="">Choose a race...</option>

          {races.map((race) => (
            <option key={race} value={race}>
              {race}
            </option>
          ))}
        </select>
      </div>

    </div>
  </div>
)}

<div className="mt-6 rounded-lg border border-white/10 bg-black/40 p-5">

  <h3 className="mb-2 font-semibold text-white">
    Appearance
  </h3>

  <p className="mb-5 text-sm text-gray-500">
    Customize your character's appearance.
  </p>

  <div className="grid gap-4 sm:grid-cols-2">

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Skin Tone
      </label>

      <select
        value={skinTone}
        onChange={(e) => setSkinTone(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Fair</option>
        <option>Light</option>
        <option>Tan</option>
        <option>Brown</option>
        <option>Dark Brown</option>
        <option>Deep</option>
      </select>
    </div>

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Hair Color
      </label>

      <select
        value={hairColor}
        onChange={(e) => setHairColor(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Black</option>
        <option>Dark Brown</option>
        <option>Brown</option>
        <option>Auburn</option>
        <option>Blonde</option>
        <option>White</option>
        <option>Silver</option>
        <option>Red</option>
      </select>
    </div>

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Hairstyle
      </label>

      <select
        value={hairStyle}
        onChange={(e) => setHairStyle(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Short</option>
        <option>Medium Length</option>
        <option>Long</option>
        <option>Messy</option>
        <option>Braided</option>
        <option>Tied Back</option>
      </select>
    </div>

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Eye Color
      </label>

      <select
        value={eyeColor}
        onChange={(e) => setEyeColor(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Brown</option>
        <option>Blue</option>
        <option>Green</option>
        <option>Gray</option>
        <option>Amber</option>
        <option>Gold</option>
      </select>
    </div>

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Body Type
      </label>

      <select
        value={bodyType}
        onChange={(e) => setBodyType(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Slender</option>
        <option>Lean</option>
        <option>Athletic</option>
        <option>Muscular</option>
        <option>Stocky</option>
      </select>
    </div>

    <div>
      <label className="mb-2 block text-sm text-gray-400">
        Height
      </label>

      <select
        value={height}
        onChange={(e) => setHeight(e.target.value)}
        className="w-full rounded-lg border border-white/10 bg-zinc-900 px-3 py-2 text-white"
      >
        <option>Short</option>
        <option>Average Height</option>
        <option>Tall</option>
        <option>Very Tall</option>
      </select>
    </div>

  </div>
</div>
<button

onClick={() => {
  if (!characterName.trim()) {
    alert("Please enter a character name.");
    return;
  }

  if (!selectedRace) {
    alert("Please choose a race.");
    return;
  }

  if (
    selectedRace === "hybrid" &&
    (!hybridRaceOne || !hybridRaceTwo)
  ) {
    alert("Please choose both hybrid ancestries.");
    return;
  }

const primaryRaceName =
  selectedRace === "hybrid"
    ? hybridRaceOne
    : selectedRace;

const primaryRace = localRaces.find(
  (race) => race.name === primaryRaceName
);

if (!primaryRace) {
  alert("Selected race could not be found.");
  return;
}

const ancestry =
  selectedRace === "hybrid"
    ? [
        localRaces.find((race) => race.name === hybridRaceOne),
        localRaces.find((race) => race.name === hybridRaceTwo),
      ].filter((race): race is typeof localRaces[number] => Boolean(race))
    : [primaryRace];

const characterAppearance: CharacterPortraitData = {
  primaryRace: primaryRace.name,
  ancestry: ancestry.map((race) => race.name),
  appearance: {
    skinTone,
    hairColor,
    hairStyle,
    eyeColor,
    bodyType,
    height,
    clothing: "adventurer clothing",
    accessories: [],
    raceFeatures: [
     ...getRaceFeatures(primaryRace.name),
     ...getAncestryFeatures(ancestry.map((race) => race.name)),
],
  },
};

setTestRace({
  primaryRace,
  ancestry,
  potential: 50,
  traits: primaryRace.traits,
  strengths: primaryRace.strengths,
  weaknesses: primaryRace.weaknesses,
});

setCharacterAppearance(characterAppearance);

setCharacterPortrait(
  generateCharacterPortrait(characterAppearance.appearance)
);

  setCharacterCreated(true);
}}

  className="mt-4 rounded-lg bg-white px-5 py-3 font-semibold text-black hover:bg-gray-200"
>
  Create Character
</button>
          

          <div className="max-w-3xl space-y-5 text-gray-300">
            <p>
              Your story has not yet begun.
            </p>

            <p>
              Create your character and step into a living world where
              races, cultures, creatures, factions, and individuals
              continue to develop whether you are watching or not.
            </p>

            <div className="rounded-lg border border-white/10 bg-black/40 p-5">
            {characterPortrait && (
  <div className="mb-5 flex justify-center">
    <div
      className="overflow-hidden rounded-xl border border-white/10 bg-black"
      dangerouslySetInnerHTML={{ __html: characterPortrait }}
    />
  </div>
)} 
 <h3 className="mb-3 font-semibold text-white">
                World Rules
              </h3>

              <ul className="space-y-2 text-sm text-gray-400">
                <li>• Humans, elves, dwarves, orcs and other races are common.</li>
                <li>• Mixed-race people exist but are uncommon.</li>
                <li>• Four-race offspring are extraordinarily rare.</li>
                <li>• A character's ancestry influences their potential.</li>
                <li>• Your choices can permanently change the world.</li>
              </ul>
            </div>
            {characterAppearance && (
  <div className="rounded-lg border border-white/10 bg-black/40 p-5">
    <h3 className="mb-3 font-semibold text-white">
      Generated Character Appearance
    </h3>

    <div className="space-y-2 text-sm text-gray-300">
      <p>
        <span className="text-gray-500">Skin:</span>{" "}
        {characterAppearance.appearance.skinTone}
      </p>

      <p>
        <span className="text-gray-500">Hair:</span>{" "}
        {characterAppearance.appearance.hairColor},{" "}
        {characterAppearance.appearance.hairStyle}
      </p>

      <p>
        <span className="text-gray-500">Eyes:</span>{" "}
        {characterAppearance.appearance.eyeColor}
      </p>

      <p>
        <span className="text-gray-500">Build:</span>{" "}
        {characterAppearance.appearance.bodyType}
      </p>

      <p>
        <span className="text-gray-500">Height:</span>{" "}
        {characterAppearance.appearance.height}
      </p>

      <p>
        <span className="text-gray-500">Clothing:</span>{" "}
        {characterAppearance.appearance.clothing}
      </p>

      <p>
        <span className="text-gray-500">Race Features:</span>{" "}
        {characterAppearance.appearance.raceFeatures.length > 0
          ? characterAppearance.appearance.raceFeatures.join(", ")
          : "None"}
      </p>
    </div>
  </div>
)}
{testRace && (
<div className="rounded-lg border border-white/10 bg-black/40 p-5">
  <h3 className="mb-3 font-semibold text-white">
    Generated Character Race
  </h3>

  <div className="space-y-2 text-sm text-gray-300">
    <p>
      <span className="text-gray-500">Primary Race:</span>{" "}
      {testRace.primaryRace.name}
    </p>

    <p>
      <span className="text-gray-500">Ancestry:</span>{" "}
      {testRace.ancestry.map((race) => race.name).join(" / ")}
    </p>

    <p>
      <span className="text-gray-500">Potential:</span>{" "}
      {testRace.potential}/100
    </p>

    <p>
      <span className="text-gray-500">Traits:</span>{" "}
      {testRace.traits.join(", ")}
    </p>

    <p>
      <span className="text-gray-500">Strengths:</span>{" "}
      {testRace.strengths.join(", ")}
    </p>

    <p>
      <span className="text-gray-500">Weaknesses:</span>{" "}
      {testRace.weaknesses.join(", ")}
    </p>
  </div>
</div>
)}
            <div className="hidden">
              <h3 className="mb-3 font-semibold text-white">
                Supabase Connection Test
              </h3>

              {error ? (
                <p className="text-red-400">
                  Connection error: {error}
                </p>
              ) : races.length > 0 ? (
                <ul className="space-y-1 text-gray-300">
                  {races.map((race) => (
                    <li key={race}>• {race}</li>
                  ))}
                </ul>
              ) : (
                <p className="text-gray-400">
                  Loading races...
                </p>
              )}
            </div>
          </div>

          <div className="mt-10">
            <label className="mb-2 block text-sm text-gray-400">
              What do you want to do?
            </label>

            <div className="flex gap-3">
              <input
                value={action}
                onChange={(e) => setAction(e.target.value)}
                placeholder="Enter an action..."
                className="flex-1 rounded-lg border border-white/10 bg-zinc-900 px-4 py-3 outline-none focus:border-white/30"
              />

              <button
                onClick={() => setAction("")}
                className="rounded-lg bg-white px-5 py-3 font-semibold text-black hover:bg-gray-200"
              >
                Act
              </button>
            </div>
          </div>
        </section>

        <aside className="rounded-xl border border-white/10 bg-zinc-950 p-5">
          <h2 className="mb-5 text-lg font-semibold">
            Character
          </h2>

<div className="space-y-4 text-sm">

<div>
  <p className="text-gray-500">Name</p>
  <p>{characterCreated ? characterName : "Not created"}</p>
</div>

            <div>
              <p className="text-gray-500">Race</p>
              <p>
                {characterCreated && testRace
      ? testRace.primaryRace.name
      : "Unknown"}
              </p>
            </div>

            <div>
              <p className="text-gray-500">Ancestry</p>
              <p>{testRace ? testRace.ancestry.map((race) => race.name).join(" / ") : "Unknown"}</p>
            </div>

            <div>
              <p className="text-gray-500">Potential</p>
              <p>{testRace ? `${testRace.potential}/100` : "Undetermined"}</p>
              </div>  
<div>
  <p className="text-gray-500">Known Factions</p>
  <p>{factions.length} discovered</p>

  <div className="mt-3 space-y-2">
    {factions.map((faction) => (
      <button
  key={faction.id}
onClick={() => setSelectedFaction(faction.id)}
  className="w-full rounded-md border border-white/10 bg-black/30 p-2 text-left hover:bg-white/10"
>
       
 <p className="font-medium text-white">
          {faction.name}
        </p>

        <p className="text-xs capitalize text-gray-500">
          {faction.type}
        </p>
      </button>
    ))}
  </div>
</div>

{selectedFaction && (() => {
  const faction = factions.find(
    (f) => f.id === selectedFaction
  );

  if (!faction) return null;

  return (
    <div className="mt-4 rounded-lg border border-white/10 bg-black/40 p-4">
      <h3 className="text-lg font-semibold text-white">
        {faction.name}
      </h3>

      <p className="mt-1 text-xs uppercase text-gray-500">
        {faction.type}
      </p>

      <p className="mt-3 text-sm text-gray-300">
        {faction.description}
      </p>

      <div className="mt-4 space-y-2 text-sm">
        <p>
          <span className="text-gray-500">Leader:</span>{" "}
          {faction.leader}
        </p>

        <p>
          <span className="text-gray-500">Power:</span>{" "}
          {faction.power}
        </p>

        <p>
          <span className="text-gray-500">Reputation:</span>{" "}
          {faction.reputation}
        </p>

        <p>
          <span className="text-gray-500">Relationship:</span>{" "}
          {faction.playerRelationship}
        </p>
      </div>
    </div>
  );
})()}
          </div>
        </aside>

      </div>
    </main>
  );
}
