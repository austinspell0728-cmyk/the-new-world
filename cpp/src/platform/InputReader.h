// InputReader.h
//
// Translates the keyboard and mouse (via raylib) into requests the rest of
// the program understands. This is the only place that knows which keys and
// mouse movements do what.

#pragma once

#include "../game/Game.h"

namespace platform {

// Reads WASD right now and returns the requested movement as a world
// direction, relative to the camera: W walks away from the camera, D walks
// to its right, and so on. `cameraYaw` is CameraRig::yaw().
// Pressing opposite keys together (e.g. A + D) cancels out on that axis.
game::PlayerInput readPlayerInput(float cameraYaw);

// How much the player wants to turn the camera this frame, in radians.
//   yawDelta    positive turns the view left, negative turns it right
//   pitchDelta  positive tilts the view further down, negative tilts it up
struct CameraInput {
    float yawDelta = 0.0f;
    float pitchDelta = 0.0f;
};

// Reads mouse movement plus Q/E and the arrow keys. `frameTime` (seconds)
// makes key turning speed independent of the frame rate; mouse movement
// already is.
CameraInput readCameraInput(float frameTime);

} // namespace platform
