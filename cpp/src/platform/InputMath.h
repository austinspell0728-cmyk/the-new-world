// InputMath.h
//
// The maths between raw device readings and the requests the rest of the
// program uses: gamepad dead zone and response curve, sticks to movement and
// camera turns, combining them with keyboard and mouse, and which device was
// used last (for the on-screen hints).
//
// Plain floats, NO raylib, so the tests check all of it without raylib or
// a gamepad. InputReader.cpp reads the devices and calls these.
//
// Stick convention here: x = right, y = UP (pushed away from you), each
// -1..1. (raylib reports up as -1; InputReader flips it before calling in.)

#pragma once

namespace platform {

struct Stick {
    float x = 0.0f;
    float y = 0.0f;
};

// Radial dead zone. The stick's distance from the centre (capped at 1, so a
// diagonal is never "more than full") is:
//   - at or inside `deadZone`: zero (worn sticks never drift);
//   - outside: rescaled from deadZone..1 to 0..1, so the output starts at 0
//     right at the edge instead of jumping to `deadZone`.
// The direction is kept. NaN input counts as centred.
Stick applyRadialDeadZone(Stick raw, float deadZone);

// Response curve for a 0..1 amount: amount ^ exponent. 1 = linear; above 1
// makes small tilts finer while full tilt stays 1. Monotonic; 0 -> 0,
// 1 -> 1. `exponent` below 1 is treated as 1.
float responseCurve(float amount, float exponent);

// Dead zone, then the curve on the stick's distance (direction kept).
// The result's length is 0..1.
Stick shapeStick(Stick raw, float deadZone, float curveExponent);

// "How far forward / right" the player asks to walk, from the camera's
// point of view (see CameraRelativeMove.h).
struct MoveAxes {
    float forward = 0.0f;
    float right = 0.0f;
};

// A shaped move stick as movement: up = forward, right = right.
MoveAxes stickToMove(Stick shaped);

// Keyboard and gamepad together: the two are simply added. With the stick
// centred this returns exactly the keyboard's values, so keyboard play is
// unchanged. Game::step normalizes the final direction, so pushing both
// never walks faster.
MoveAxes combineMove(MoveAxes keyboard, MoveAxes gamepad);

// How much to turn the camera this frame, in radians (same signs as
// CameraInput in InputReader.h: yaw + turns the view left, pitch + tilts it
// further down).
struct CameraTurn {
    float yawDelta = 0.0f;
    float pitchDelta = 0.0f;
};

// A shaped camera stick as a turn: right turns the view right, up tilts it
// up (down with `invertY`). Full tilt turns `degreesPerSecond`.
CameraTurn stickToCameraTurn(Stick shaped, float degreesPerSecond, bool invertY, float frameTime);

// Mouse/keys and gamepad turns together: added. With the stick centred this
// returns exactly the mouse/keys turn.
CameraTurn combineTurn(CameraTurn mouseAndKeys, CameraTurn gamepad);

// Which device the on-screen hints should talk about.
enum class InputDevice { KeyboardMouse, Gamepad };

// Remembers the device used last. Each frame, report whether the keyboard
// or mouse was used and whether the gamepad was used (a button or a stick
// outside its dead zone) and whether a pad is connected at all:
//   - only one used: that one becomes the last used;
//   - both or neither: no change;
//   - no pad connected: back to keyboard/mouse.
// Starts on keyboard/mouse.
class LastUsedDevice {
public:
    InputDevice update(bool keyboardMouseUsed, bool gamepadUsed, bool gamepadConnected);
    InputDevice current() const { return current_; }

private:
    InputDevice current_ = InputDevice::KeyboardMouse;
};

} // namespace platform
