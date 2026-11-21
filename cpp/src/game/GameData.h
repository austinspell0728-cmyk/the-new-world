// GameData.h
//
// Tunable game data: numbers and content that designers would tweak.
//
// Everything here is measured in WORLD UNITS, where 1 unit = 1 tile.
// Nothing in this file knows about pixels, windows or colors; those belong
// to the renderer (see platform/RenderConfig.h).

#pragma once

#include <array>
#include <string_view>

namespace game::data {

// --- Simulation timing -------------------------------------------------------

// The world is updated in fixed steps of this length (seconds), no matter how
// fast the screen refreshes. Fixed steps keep movement identical on slow and
// fast machines, matching the deterministic goal of the TypeScript engine.
constexpr float kFixedTimeStep = 1.0f / 60.0f;

// --- Player ------------------------------------------------------------------

// Walking speed in tiles per second.
constexpr float kPlayerWalkSpeed = 5.0f;

// Half the width of the player's square footprint on the ground (X/Z), in
// tiles. Collision uses this footprint only. 0.35 makes the player a little
// smaller than one tile so it fits through one-tile-wide gaps without snagging.
constexpr float kPlayerHalfSize = 0.35f;

// How tall the player stands, in tiles (1 tile is roughly 1 metre). Used by
// the renderer to size the body and by the camera to know where the head is;
// collision does not use it yet because there is no vertical movement.
constexpr float kPlayerHeight = 1.7f;

// --- Starting world ----------------------------------------------------------

// The starting map seen from above, one string per row, read top to bottom.
// Each character is one tile: its column is the X coordinate and its row is
// the Z coordinate, so the first row is the north edge (smallest Z).
//   '#'  wall (blocks movement)
//   '~'  water (blocks movement)
//   '.'  grass (walkable)
//   ','  dirt path (walkable)
//   '@'  player start position (walkable grass)
//
// All rows must be the same length; World checks this when it loads.
constexpr std::array<std::string_view, 20> kStartingMap = {
    "########################################",
    "#..........,,,,,,,,....................#",
    "#..........,......,....~~~~~~..........#",
    "#..........,......,...~~~~~~~~.........#",
    "#...####...,......,...~~~~~~~~.........#",
    "#...#..#...,......,....~~~~~~..........#",
    "#...#..#...,,,,@,,,,,,,,,,,,,,,,,,,....#",
    "#...##.#...........................,...#",
    "#..................................,...#",
    "#.........#########................,...#",
    "#.........#.......#................,...#",
    "#.........#.......#.......~~~......,...#",
    "#.........####.####......~~~~~.....,...#",
    "#........................~~~~~.....,...#",
    "#.........................~~~......,...#",
    "#..................................,...#",
    "#,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,...#",
    "#......................................#",
    "#......................................#",
    "########################################",
};

} // namespace game::data
