// CameraRig.cpp
//
// World units (tiles) map 1:1 onto raylib's 3D units, so game positions
// can be handed to the camera unchanged.

#include "CameraRig.h"

#include <cmath>

#include "RenderConfig.h"

namespace platform {

CameraRig::CameraRig()
    : yaw_(render::kCameraStartYawDeg * DEG2RAD), pitch_(render::kCameraStartPitchDeg * DEG2RAD) {
    camera_.up = {0.0f, 1.0f, 0.0f}; // Y is up
    camera_.fovy = render::kCameraFovY;
    camera_.projection = CAMERA_PERSPECTIVE;
}

void CameraRig::follow(game::Vec3 playerFeet) {
    // The point we look at: a little above the player's feet.
    const Vector3 target{playerFeet.x, playerFeet.y + render::kCameraLookHeight, playerFeet.z};

    // Direction the camera faces across the ground (unit length).
    const float forwardX = std::sin(yaw_);
    const float forwardZ = std::cos(yaw_);

    // Split the fixed distance into a horizontal part (how far behind) and
    // a vertical part (how far above), based on the pitch angle.
    const float behind = render::kCameraDistance * std::cos(pitch_);
    const float above = render::kCameraDistance * std::sin(pitch_);

    // Stand `behind` units opposite to the facing direction, `above` units up.
    camera_.position = {target.x - forwardX * behind, target.y + above,
                        target.z - forwardZ * behind};
    camera_.target = target;
}

} // namespace platform
