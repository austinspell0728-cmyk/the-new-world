// CameraRelativeMove.h
//
// Turns "forward / right" key presses into a world-space walking direction,
// based on which way the camera faces.
//
// This lives in platform/ because it is about how controls map onto the
// view, not a game rule: the game just receives a world direction.
// It is plain math with NO raylib include, so the test build can check it
// without linking raylib.

#pragma once

#include <cmath>

#include "../game/Game.h"

namespace platform {

// forwardAmount: +1 for W (away from the camera), -1 for S.
// rightAmount:   +1 for D (to the camera's right), -1 for A.
// cameraYaw:     camera facing in radians, measured like Player::facingYaw
//                (0 faces +Z, pi/2 faces +X).
//
// Returns the matching direction on the ground. It is NOT normalized:
// W + D gives a vector of length ~1.41, and Game::step normalizes it so
// diagonal movement is no faster than straight movement.
inline game::PlayerInput cameraRelativeMove(float forwardAmount, float rightAmount,
                                            float cameraYaw) {
    // The camera's forward direction across the ground.
    const float forwardX = std::sin(cameraYaw);
    const float forwardZ = std::cos(cameraYaw);

    // Its right-hand direction: forward turned 90 degrees clockwise when
    // seen from above (with Y up, that is (-forwardZ, forwardX)).
    // Example: facing -Z (north), right is +X (east).
    const float rightX = -forwardZ;
    const float rightZ = forwardX;

    game::PlayerInput input;
    input.moveX = forwardX * forwardAmount + rightX * rightAmount;
    input.moveZ = forwardZ * forwardAmount + rightZ * rightAmount;
    return input;
}

} // namespace platform
