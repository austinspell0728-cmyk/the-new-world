# The New World: C++ prototype

A small walkable town seen from a third-person camera: buildings, a market
square, trees, a pond and a few townsfolk you can talk to. Built with C++17
and [raylib](https://www.raylib.com/). It lives entirely in `cpp/` and does
not touch the Next.js app.

All art is placeholder: flat-colored boxes and capsules.

## Build and run

You need a C++17 compiler, GNU make and a local raylib install (raylib is not
committed to this repo).

```sh
cd cpp
make RAYLIB_PATH=/path/to/raylib/src   # default: C:/raylib/raylib/src
make run                               # build and start the game
make test                              # rule tests, no raylib needed
make clean
```

Developer flags (both ignore the camera controls):

```sh
./build/new-world --screenshot build/shot.png   # save one frame and quit
./build/new-world --benchmark                   # draw uncapped, print average FPS, quit
```

The game reads `data/npcs.txt` at startup. It looks in `data/` under the
folder you start it from (`cpp/` with `make run`), then in `../data/` next to
the exe, so `cpp/build/new-world` also works when started from elsewhere.

## Controls

| Input | Action |
|---|---|
| **W A S D** | Walk, relative to the camera (W = away from the camera) |
| **Mouse** | Orbit the camera around the player (the cursor is captured) |
| **Left / Right arrows** | Turn the camera |
| **Up / Down arrows** | Tilt the camera |
| **E** | Talk to the NPC next to you; press again to close |
| **Tab** | Release the mouse to use other windows; Tab again to recapture |
| **Esc** | Quit |

## The town

- `src/game/GameData.h` holds the town as a text map (`kTownLayout`), with a
  legend: `B` building, `T` tree, `S` market stall, `W` well, `:` market
  paving, `,` path, `~` water, `#` wall, `.` grass, `@` player start, and
  `' '` for "keep the terrain underneath". Touching `B` tiles form one
  building; a door is drawn wherever a path or the square meets it.
- Every solid tile (building, tree, stall, well, water, wall) blocks its
  whole square, and so does each NPC's tile.
- `data/npcs.txt` holds the NPCs: name, tile, facing, color and one line
  each. Edit it and re-run; no rebuild needed. Mistakes (a typo'd key, an
  NPC inside a building) stop the game with the file, line and problem.

### Where seeded terrain generation goes

The world is built in layers in `src/game/WorldBuilder.*`:

1. `generateTerrain(seed, width, depth)`: today flat grass inside a border
   wall, ignoring the seed. **Replace its body with a seeded generator.**
   Keep it deterministic: the same seed must give the same rows.
2. `stampLayout(...)`: hand-made layouts (the town) go on top; spaces keep
   the generated terrain.
3. `World::fromRows`: validates the result.

`GameData::kWorldSeed` is already passed through from `main.cpp`.

## Coordinates

- The ground is the **X/Z plane** and **Y points up** (0 = ground level).
- The text map's columns are X and its rows are Z, so the first row of the
  map is the north edge. The camera starts south of the player looking north,
  so the world appears the same way round as the text map.
- Everything is measured in **tiles** (1 tile = 1 unit, roughly 1 metre).
  raylib's 3D units are used 1:1, so game positions go to the renderer
  unchanged.
- Collision is still 2D: the player's square footprint against blocked
  tiles on the X/Z plane. Height is not simulated.

## Layout

```
data/
  npcs.txt       NPC names, positions, colors and lines (loaded at runtime)
src/
  game/          rules and data, plain C++, never includes raylib
    GameData.h     tunable numbers, world size and the town layout
    WorldBuilder.* terrain layer (seeded generation hook) + layout stamping
    World.*        tile grid, map loading, which tiles are solid
    Npc.h          one NPC
    NpcData.*      reads the NPC text format
    Game.*         player, movement, collision, NPCs, talking, fixed 60 Hz update
    Vec3.h         small 3D vector type and ground-plane helpers
  platform/      window, input, camera and drawing
    InputReader.*        keys and mouse -> movement, camera turns, E, Tab
    CameraRelativeMove.h WASD + camera yaw -> world direction (no raylib)
    CameraRig.*          third-person orbit camera
    WorldMesh.*          the static map baked into one GPU mesh
    Renderer.*           draws the world, characters, labels, dialogue, HUD
    RenderConfig.h       window, camera, shape sizes and colors
  main.cpp       main loop: turn camera -> read input -> step -> draw
tests/
  GameTests.cpp  43 tests for movement, map, town, NPC and control rules
```

The rule of thumb: `game/` decides *what happens*; `platform/` decides *how
it looks and which key does what*. `game/` takes a plain world direction and
an `interact()` call, and never knows a camera or keyboard exists. Because
`make test` builds without raylib, a raylib include slipping into `game/`
breaks the test build straight away.

## Known limits

- The camera has no collision, so it can pass through buildings.
- No jumping, falling or slopes; everyone stands at y = 0.
- Props and NPCs block their whole tile, even though they are drawn smaller.
- NPCs stand still; dialogue is one line each, no choices.
