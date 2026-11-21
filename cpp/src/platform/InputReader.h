// InputReader.h
//
// Translates physical keys (via raylib) into the game's PlayerInput.
// This is the only place that knows which keys do what.

#pragma once

#include "../game/Game.h"

namespace platform {

// Reads the keyboard right now and returns the requested movement.
// WASD and the arrow keys both work. Pressing opposite keys together
// (e.g. left + right) cancels out to 0 on that axis.
game::PlayerInput readPlayerInput();

} // namespace platform
