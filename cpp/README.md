# The New World: C++ prototype

A 3D tile world seen from a third-person camera, with a player who can walk
around. Built with C++17 and [raylib](https://www.raylib.com/). It lives
entirely in `cpp/` and does not touch the Next.js app.

All art is placeholder: flat-colored boxes and a capsule.

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

To check the view without playing, save one frame and quit:

```sh
./build/new-world --screenshot build/shot.png
```

## Controls

| Input | Action |
|---|---|
| **W A S D** | Walk, relative to the camera (W = away from the camera) |
| **Mouse** | Orbit the camera around the player (the cursor is captured) |
| **Q / E**, **Left / Right arrows** | Turn the camera left / right |
| **Up / Down arrows** | Tilt the camera up / down |
| **Esc** | Quit |

## Coordinates

- The ground is the **X/Z plane** and **Y points up** (0 = ground level).
- The text map's columns are X and its rows are Z, so the first row of the
  map is the north edge. The camera starts south of the player looking north,
  so the world appears the same way round as the text map.
- Everything is measured in **tiles** (1 tile = 1 unit, roughly 1 metre).
  raylib's 3D units are used 1:1, so game positions go to the renderer
  unchanged.
- Collision is still 2D: the player's square footprint against solid tiles
  on the X/Z plane. Height is not simulated yet.

## Layout

```
src/
  game/          rules and data, plain C++, never includes raylib
    GameData.h     tunable numbers (speed, player size/height) and the map
    World.*        tile grid, map loading, which tiles are solid
    Game.*         player, movement, collision, fixed 60 Hz update
    Vec3.h         small 3D vector type and ground-plane helpers
  platform/      window, input, camera and drawing
    InputReader.*        keys and mouse -> movement and camera turns
    CameraRelativeMove.h WASD + camera yaw -> world direction (no raylib)
    CameraRig.*          third-person orbit camera
    Renderer.*           draws the 3D world, player and HUD
    RenderConfig.h       window, camera, shape sizes and colors
  main.cpp       main loop: turn camera -> read movement -> step -> draw
tests/
  GameTests.cpp  23 tests for movement, map and control rules (no raylib)
```

The rule of thumb: `game/` decides *what happens*; `platform/` decides *how
it looks and which key does what*. `game/` takes a plain world direction and
never knows a camera exists. Turning WASD into that direction happens in
`platform/CameraRelativeMove.h`, which is plain math so it can be tested too.
Because `make test` builds without raylib, a raylib include slipping into
`game/` (or that header) breaks the test build straight away.

## Editing the map

Change `kStartingMap` in `src/game/GameData.h`:
`#` wall, `~` water, `.` grass, `,` path, `@` player start (exactly one).
Rows must all be the same length; the game reports a clear error otherwise.
Walls are drawn as boxes `kWallHeight` tall (see `RenderConfig.h`).

## Known limits

- The camera has no collision, so it can pass through walls.
- No jumping, falling or slopes; the player always stands at y = 0.
- About 55 FPS on the dev machine while drawing ~800 tiles one by one.
  Gameplay is unaffected (fixed 60 Hz steps); batching the tiles into a
  single mesh would fix it if it becomes a problem.
