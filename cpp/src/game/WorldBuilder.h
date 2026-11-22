// WorldBuilder.h
//
// How the starting world is put together, in layers:
//
//   1. TERRAIN   generateTerrain(seed, width, depth)
//                The ground the whole world sits on. Today it is flat grass
//                inside a border wall and the seed is ignored.
//                >>> Seeded terrain generation plugs in HERE. <<<
//                Replace the body of generateTerrain with a generator that
//                uses the seed (water, forests, rocks...). Keep it
//                deterministic: the same seed must give the same rows, so a
//                saved seed rebuilds the same world (like the TypeScript
//                engine's seeded random generator).
//
//   2. LAYOUTS   stampLayout(rows, layout, offsetX, offsetZ)
//                Hand-made places (today: the town in GameData.h) are
//                stamped on top. A space ' ' in a layout means "keep the
//                terrain underneath", so generated terrain shows around
//                and between hand-made content.
//
//   3. CHECK     World::fromRows validates the result (size, characters,
//                exactly one '@') and turns it into tiles.
//
// Everything works on rows of map characters (the same legend as
// GameData.h), because text is easy to generate, stamp, print and test.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "World.h"

namespace game {

// The world as editable rows of map characters, before it becomes a World.
using MapRows = std::vector<std::string>;

// Layer 1: the terrain. Returns `depth` rows of `width` characters.
// Throws std::invalid_argument if the size is smaller than 3x3.
MapRows generateTerrain(std::uint32_t seed, int width, int depth);

// Layer 2: copies `layout` onto `rows` with its top-left corner at
// (offsetX, offsetZ). Spaces in the layout are skipped. Throws
// std::invalid_argument if the layout does not fit inside `rows` or its
// rows have different lengths.
void stampLayout(MapRows& rows, const std::vector<std::string_view>& layout, int offsetX,
                 int offsetZ);

// The whole pipeline for the starting world, using the sizes, town layout
// and offsets in GameData.h.
World buildStartingWorld(std::uint32_t seed);

} // namespace game
