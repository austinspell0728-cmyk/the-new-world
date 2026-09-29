"use client";

import { useEffect, useState } from "react";
import { supabase } from "../lib/supabase";
import { factions } from "../lib/game/factions";
export default function Home() {
  const [action, setAction] = useState("");
  const [races, setRaces] = useState<string[]>([]);
  const [error, setError] = useState("");

  useEffect(() => {
    async function loadRaces() {
      const { data, error } = await supabase
        .from("races")
        .select("name");

     if (error) {
  console.error("Supabase error:", error);
  setError(error.message);
  return;
}

console.log("Supabase races:", data);
setRaces(data.map((race) => race.name));

      setRaces(data.map((race) => race.name));
    }

    loadRaces();
  }, []);

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

            <div className="rounded-lg border border-white/10 bg-black/40 p-5">
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
              <p>Not created</p>
            </div>

            <div>
              <p className="text-gray-500">Race</p>
              <p>Unknown</p>
            </div>

            <div>
              <p className="text-gray-500">Ancestry</p>
              <p>Unknown</p>
            </div>

            <div>
              <p className="text-gray-500">Potential</p>
              <p>Undetermined</p>
<div>
  <p className="text-gray-500">Known Factions</p>
  <p>{factions.length} discovered</p>

  <div className="mt-3 space-y-2">
    {factions.map((faction) => (
      <div
        key={faction.id}
        className="rounded-md border border-white/10 bg-black/30 p-2"
      >
        <p className="font-medium text-white">
          {faction.name}
        </p>

        <p className="text-xs capitalize text-gray-500">
          {faction.type}
        </p>
      </div>
    ))}
  </div>
</div>
            </div>
          </div>
        </aside>

      </div>
    </main>
  );
}
