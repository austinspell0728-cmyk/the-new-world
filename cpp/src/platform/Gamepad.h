// Gamepad.h
//
// Reads a gamepad through raylib, if one is plugged in, and turns it into
// shaped sticks and "pressed this frame" actions (see InputMath.h and
// data/controls.txt). The rest of the program combines this with the
// keyboard and mouse, so the game never knows which device was used.
//
// Hot-plug: every frame it looks for the first connected pad, so plugging
// one in or pulling it out mid-game just switches it on or off. It says so
// once in the console when that happens.
//
// Scripted test pad: with a FakePad, no hardware is read; the given stick
// values are fed in every frame instead (for testing the movement and
// camera paths without a controller; see --fake-pad in main.cpp).

#pragma once

#include <optional>
#include <string>

#include "ControlsData.h"
#include "InputMath.h"

namespace platform {

// Raw stick values for the scripted test pad: x right, y up, -1..1. They go
// through the same dead zone and curve as a real pad.
struct FakePad {
    Stick move;
    Stick camera;
};

// One frame of gamepad input.
struct GamepadFrame {
    bool connected = false;
    std::string name;      // as the driver reports it
    Stick move;            // shaped: dead zone and curve applied
    Stick camera;          // shaped
    bool interactPressed = false;
    bool pausePressed = false;
    bool used = false;     // any button, or a stick outside its dead zone
};

class GamepadReader {
public:
    // `enabled` false: never reads a pad (screenshot and benchmark runs stay
    // reproducible). A `fake` pad works even when not enabled.
    GamepadReader(const ControlsData& controls, bool enabled, std::optional<FakePad> fake);

    // Reads this frame. Call once per frame, after the window is open. The
    // first call prints one line saying whether a pad was found.
    GamepadFrame read();

private:
    ControlsData controls_;
    bool enabled_;
    std::optional<FakePad> fake_;
    int padIndex_ = -1;      // the pad in use, -1 = none
    bool announced_ = false; // printed the startup line yet
};

} // namespace platform
