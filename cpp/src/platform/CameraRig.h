// CameraRig.h
//
// The third-person camera: it keeps a fixed distance from the player and
// sits on a sphere around them, described by two angles:
//
//   yaw    which way the camera faces across the ground (around the Y axis),
//          measured like Player::facingYaw: 0 faces +Z, pi/2 faces +X.
//   pitch  how steeply the camera looks down at the player.
//
// The camera is presentation only. The game never sees it; the platform
// layer uses its yaw to turn WASD into a world direction.

#pragma once

#include "raylib.h"

#include "../game/Vec3.h"

namespace platform {

class CameraRig {
public:
    CameraRig();

    // Turns the camera around the player by the given angles (radians).
    // Pitch is clamped to RenderConfig's limits so the camera can never go
    // under the ground or flip over the top. There is no camera collision
    // yet, so it can pass through walls.
    void orbit(float yawDelta, float pitchDelta);

    // Moves the camera so it looks at `playerFeet` from the current angles.
    // Call once per frame after the game has updated.
    void follow(game::Vec3 playerFeet);

    // The raylib camera to pass to BeginMode3D.
    const Camera3D& camera() const { return camera_; }

    // Current angles, in radians.
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }

private:
    float yaw_;
    float pitch_;
    Camera3D camera_{};
};

} // namespace platform
