// InputReader.h
//
// Translates the keyboard and mouse (via raylib) into requests the rest of
// the program understands. This is the only place that knows which keys and
// mouse movements do what. (The gamepad is read in Gamepad.h; main.cpp adds
// the two together with InputMath.h, so either can be used at any time.)

#pragma once

#include "InputMath.h"

namespace platform {

// Reads WASD right now as "forward / right" from the camera's point of
// view: W = forward +1, S = -1, D = right +1, A = -1. Pressing opposite keys
// together (e.g. A + D) cancels out on that axis. CameraRelativeMove.h turns
// it into a world direction.
MoveAxes readKeyboardMove();

// How much the player wants to turn the camera this frame, in radians.
//   yawDelta    positive turns the view left, negative turns it right
//   pitchDelta  positive tilts the view further down, negative tilts it up
struct CameraInput {
    float yawDelta = 0.0f;
    float pitchDelta = 0.0f;
};

// Reads mouse movement plus the arrow keys. `frameTime` (seconds)
// makes key turning speed independent of the frame rate; mouse movement
// already is. With `useMouse` false (the mouse is released), only the keys
// turn the camera.
CameraInput readCameraInput(float frameTime, bool useMouse);

// True on the frame Tab is pressed: release or recapture the mouse.
bool toggleMouseCapturePressed();

// True on the frame E is pressed: talk to the nearby NPC (or stop talking).
bool interactPressed();

// True if the keyboard or mouse was used this frame: one of the game's keys
// held, a mouse button pressed, or (while captured) the mouse moved. For
// switching the on-screen hints between keyboard and gamepad.
bool keyboardOrMouseUsed(bool mouseCaptured);

} // namespace platform
