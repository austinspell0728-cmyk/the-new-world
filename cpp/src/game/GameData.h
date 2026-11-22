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

// --- NPCs ----------------------------------------------------------------------

// NPCs are the same size as the player. Their names, positions, colors and
// lines are NOT here: they are loaded at runtime from data/npcs.txt.
constexpr float kNpcHalfSize = 0.35f;
constexpr float kNpcHeight = 1.7f;

// How close (in tiles, centre to centre on the ground) the player must be to
// talk to an NPC with E. Walking further than this away closes the dialogue.
constexpr float kTalkRange = 1.6f;

// --- Camera occlusion (see CameraOcclusion.h) -----------------------------------
//
// How tall things in the town are, for keeping the camera out of them. In
// tiles (1 tile = 1 m). Rounded UP from the drawn models on purpose: it is
// better to pull the camera in a little early than to let it into a roof.

// Building tiles block from the ground up to the top of the tallest roof
// ridge (the kit's 6-wide roofs reach about 7.9 m).
constexpr float kOccluderBuildingTop = 8.0f;
// Roofs overhang their walls by about a tile. In the tiles around a
// building (sides and corners), the eaves block between these heights.
constexpr float kOccluderEavesBottom = 2.0f;
constexpr float kOccluderEavesTop = 3.6f;
// Tree tiles block from the ground to the top of the crown (trees are up
// to about 7.5 m). Crowns spread about a tile further: in the tiles around a
// tree they block between these heights. The bottom is kept above head
// height so the camera can still sit behind a player standing under a crown.
constexpr float kOccluderTreeTop = 7.5f;
constexpr float kOccluderCrownBottom = 2.2f;
// Low things that block from the ground up: market stalls, the well and the
// stone border wall.
constexpr float kOccluderStallTop = 1.9f;
constexpr float kOccluderWellTop = 1.8f;
constexpr float kOccluderWallTop = 2.0f;

// The camera keeps this much room (in tiles, around its position) from
// anything that blocks, so the near edge of the view does not cut into a
// wall or leaves.
constexpr float kCameraClearance = 0.4f;
// It never comes closer to the point it looks at than this, unless that
// would put it inside something.
constexpr float kCameraMinDistance = 1.0f;
// And never closer than this, even if that point is inside something: a
// camera sitting on the point it looks at cannot draw anything (the view
// direction is undefined). 0.3 is inside the player's own half-width
// (0.35), so with the player pressed against a wall it still stays out of
// the wall.
constexpr float kCameraNearestDistance = 0.3f;
// How finely the line from the player to the camera is checked, in tiles.
constexpr float kCameraSampleStep = 0.05f;
// How long a full pull-in (from the normal distance to the minimum) and a
// full release take, in seconds. The camera distance changes at a steady
// rate, so it never jumps.
constexpr float kCameraPullInSeconds = 0.10f;
constexpr float kCameraReleaseSeconds = 0.15f;

// --- Starting world ----------------------------------------------------------
//
// The world is built in two layers (see WorldBuilder.h):
//   1. terrain, generated from kWorldSeed (today: plain grass inside a wall)
//   2. the hand-made town below, stamped on top at kTownOffsetX/Z

// Seed for terrain generation. Not used yet; it is passed through so seeded
// generation can be added without changing any callers.
constexpr unsigned int kWorldSeed = 1;

// Size of the whole world, in tiles.
constexpr int kWorldWidth = 48; // along X
constexpr int kWorldDepth = 36; // along Z

// Where the town's top-left corner lands in the world.
constexpr int kTownOffsetX = 2;
constexpr int kTownOffsetZ = 2;

// The town seen from above, one string per row, read top to bottom.
// Each character is one tile: its column is X and its row is Z (plus the
// offsets above), so the first row is the north side.
//   ' '  nothing: keep whatever terrain is underneath
//   '.'  grass            (walkable)
//   ','  dirt path        (walkable)
//   ':'  market paving    (walkable)
//   '~'  water            (blocks movement)
//   '#'  stone wall       (blocks movement)
//   'B'  building         (blocks movement; touching 'B' tiles form one building)
//   'T'  tree             (blocks movement)
//   'S'  market stall     (blocks movement)
//   'W'  well             (blocks movement)
//   '@'  player start     (walkable path; exactly one in the whole world)
//
// All rows must be the same length; the builder checks this when it loads.
constexpr std::array<std::string_view, 32> kTownLayout = {
    "                    ,,,                     ",
    "                    ,,,                     ",
    "  BBBBBB  BBBBBB T  ,,,   BBBBBBB T         ",
    "  BBBBBB  BBBBBB    ,,, T BBBBBBB        T  ",
    "  BBBBBB TBBBBBB    ,,,   BBBBBBB           ",
    "  BBBBBB  BBBBBB    ,,,   BBBBBBB           ",
    "  BBBBBB  BBBBBB    ,,,   BBBBBBB           ",
    "    ,     BBBBBB    ,,,      ,              ",
    " T  ,       ,       ,,, T    ,            T ",
    "  BBBBBBB   ,       ,,,      , T   BBBBBB   ",
    "  BBBBBBB   ,  ::::::::::::: ,     BBBBBB   ",
    "  BBBBBBB  T,  ::::::::::::: ,     BBBBBB   ",
    "  BBBBBBB   ,  :::SS:::SS::: ,     BBBBBB   ",
    "    ,,      ,  ::::::::::::: ,       ,      ",
    ",,,,,,,,,,,,,,,:::::::::::::,,,,,,,,,,,,,,,,",
    ",,,,,,,,,,,,,,,::::::W::::::,,,,,,,,,,,,,,,,",
    ",,,,,,,,,,,,,,,:::::::::::::,,,,,,,,,,,,,,,,",
    "     ,         :::::::::::::  ,       ,   T ",
    "     ,   T     :::SS:::SS:::  ,       ,     ",
    "  BBBBBBB   T  :::::::::::::  ,     BBBBBB  ",
    "  BBBBBBB      :::::::::::::  ,     BBBBBB  ",
    "  BBBBBBB      ,    ,,,       ,     BBBBBB  ",
    "  BBBBBBB      ,    ,,,    BBBBBBBB BBBBBB  ",
    "  BBBBBBB    BBBBBB ,,,    BBBBBBBB BBBBBB  ",
    "  BBBBBBB    BBBBBB ,,,    BBBBBBBB       T ",
    "          T  BBBBBB ,@,    BBBBBBBB  ~~~~~  ",
    "             BBBBBB ,,,  T BBBBBBBB ~~~~~~  ",
    " T           BBBBBB ,,,    BBBBBBBB ~~~~~~  ",
    "      T             ,,,              ~~~~~  ",
    "                  T ,,,              ~~~~~  ",
    "                    ,,,          T    ~~    ",
    "                    ,,,                     ",
};

} // namespace game::data
