# The New World: C++ prototype

A top-down tile world with a player who can walk around, built with C++17 and
[raylib](https://www.raylib.com/). It lives entirely in `cpp/` and does not
touch the Next.js app.

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

Controls: **WASD** or **arrow keys** to move, **Esc** to quit.

## Layout

```
src/
  game/          rules and data, plain C++, never includes raylib
    GameData.h     tunable numbers and the starting map
    World.*        tile grid, map loading, which tiles are solid
    Game.*         player, movement, collision, fixed-step update
    Vec2.h         small 2D vector type
  platform/      everything that touches raylib
    InputReader.*  keyboard -> PlayerInput
    Renderer.*     draws the world, player, camera and HUD
    RenderConfig.h window size, pixels per tile, colors
  main.cpp       window and main loop: input -> step -> draw
tests/
  GameTests.cpp  tests for movement and map rules (built without raylib)
```

The rule of thumb: `game/` decides *what happens*, `platform/` decides *how it
looks and which key does what*. All gameplay is in world units (1 unit = 1
tile); pixels exist only in the renderer. Because `make test` links the game
code without raylib, a raylib include slipping into `game/` breaks the test
build straight away.

## Editing the map

Change `kStartingMap` in `src/game/GameData.h`:
`#` wall, `~` water, `.` grass, `,` path, `@` player start (exactly one).
Rows must all be the same length; the game reports a clear error otherwise.
