// InputReader.cpp

#include "InputReader.h"

#include "raylib.h"

#include "CameraRelativeMove.h"
#include "RenderConfig.h"

namespace platform {

namespace {

// +1 if `positive` is held, -1 if `negative` is held, 0 for neither or both.
float axis(int negativeKey, int positiveKey) {
    return static_cast<float>(static_cast<int>(IsKeyDown(positiveKey)) -
                              static_cast<int>(IsKeyDown(negativeKey)));
}

} // namespace

game::PlayerInput readPlayerInput(float cameraYaw) {
    // The keys say "forward/right" from the camera's point of view;
    // cameraRelativeMove turns that into a direction in the world.
    return cameraRelativeMove(axis(KEY_S, KEY_W), axis(KEY_A, KEY_D), cameraYaw);
}

CameraInput readCameraInput(float frameTime, bool useMouse) {
    const float keyTurn = render::kCameraKeyTurnDegPerSec * DEG2RAD * frameTime;

    // Pixels moved since last frame. Ignored while the mouse is released,
    // so moving it over to another window does not spin the camera.
    const Vector2 mouse = useMouse ? GetMouseDelta() : Vector2{0.0f, 0.0f};

    CameraInput input;

    // Mouse right / E / Right arrow turn the view right (negative yaw).
    input.yawDelta = -mouse.x * render::kMouseSensitivity +
                     axis(KEY_E, KEY_Q) * keyTurn + axis(KEY_RIGHT, KEY_LEFT) * keyTurn;

    // Mouse down / Down arrow tilt the view further down (positive pitch).
    input.pitchDelta = mouse.y * render::kMouseSensitivity + axis(KEY_UP, KEY_DOWN) * keyTurn;

    return input;
}

bool toggleMouseCapturePressed() { return IsKeyPressed(KEY_TAB); }

} // namespace platform
