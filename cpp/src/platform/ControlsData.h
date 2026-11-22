// ControlsData.h
//
// Reads data/controls.txt: the gamepad's sticks, feel and button mapping.
// Plain C++ with NO raylib, so the tests check it without a pad; InputReader
// turns the button names into raylib's buttons. Same format and error style
// as the other data files.
//
// Example (see data/controls.txt for the real file):
//
//     [gamepad]
//     move_stick     = left
//     camera_stick   = right
//     dead_zone      = 0.2
//     response_curve = 1.5
//     camera_speed   = 150
//     invert_y       = no
//
//     [buttons]
//     interact = a
//     pause    = start
//     attack   = none
//     dodge    = none
//     cast     = none
//
// [gamepad]: which stick walks and which turns the camera (left / right,
// not both the same); dead_zone 0 to 0.9 (how far a stick moves before it
// counts); response_curve 1 to 3 (1 = linear, higher = finer small moves);
// camera_speed 10 to 720 (degrees per second at full tilt); invert_y yes or
// no (yes: pushing up tilts the view down).
//
// [buttons]: an action = a button name (see buttonNames() in the .cpp: a,
// b, x, y, lb, rb, lt, rt, back, start, ls, rs, dpad_up, dpad_down,
// dpad_left, dpad_right) or none. interact and pause are required. attack,
// dodge and cast are reserved for later: they may be listed (the game
// does not use them yet) and default to none. One button can only do one
// action.
//
// Both sections must be present, once each, with every [gamepad] key.
// Comments must be on their own line.

#pragma once

#include <istream>
#include <string>

namespace platform {

// Gamepad buttons by their Xbox-style position.
enum class PadButton {
    None,
    A, B, X, Y,          // right face: bottom, right, left, top
    LB, RB, LT, RT,      // shoulders: bumpers and triggers
    Back, Start,         // centre buttons
    LS, RS,              // stick clicks
    DpadUp, DpadDown, DpadLeft, DpadRight,
};

// The name used in controls.txt and on screen ("A", "Start"), for hints.
const char* buttonLabel(PadButton button);

enum class PadStick { Left, Right };

struct ControlsData {
    std::string source;
    PadStick moveStick = PadStick::Left;
    PadStick cameraStick = PadStick::Right;
    float deadZone = 0.2f;
    float responseCurve = 1.5f;
    float cameraSpeed = 150.0f; // degrees per second at full tilt
    bool invertY = false;

    PadButton interact = PadButton::A;
    PadButton pause = PadButton::Start;
    // Reserved for combat later; not used by the game yet.
    PadButton attack = PadButton::None;
    PadButton dodge = PadButton::None;
    PadButton cast = PadButton::None;
};

// Parses `in`. `sourceName` is used in error messages.
// Throws std::runtime_error ("source:line: problem") on bad input.
ControlsData parseControls(std::istream& in, const std::string& sourceName);

// Opens `path` and parses it. Throws std::runtime_error if the file cannot
// be opened or is invalid.
ControlsData loadControlsFile(const std::string& path);

} // namespace platform
