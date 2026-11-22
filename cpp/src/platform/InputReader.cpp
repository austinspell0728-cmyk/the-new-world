// InputReader.cpp

#include "InputReader.h"

#include "raylib.h"

#include "RenderConfig.h"

namespace platform {

namespace {

// +1 if `positive` is held, -1 if `negative` is held, 0 for neither or both.
float axis(int negativeKey, int positiveKey) {
    return static_cast<float>(static_cast<int>(IsKeyDown(positiveKey)) -
                              static_cast<int>(IsKeyDown(negativeKey)));
}

} // namespace

MoveAxes readKeyboardMove() { return {axis(KEY_S, KEY_W), axis(KEY_A, KEY_D)}; }

CameraInput readCameraInput(float frameTime, bool useMouse) {
    const float keyTurn = render::kCameraKeyTurnDegPerSec * DEG2RAD * frameTime;

    // Pixels moved since last frame. Ignored while the mouse is released,
    // so moving it over to another window does not spin the camera.
    const Vector2 mouse = useMouse ? GetMouseDelta() : Vector2{0.0f, 0.0f};

    CameraInput input;

    // Mouse right / Right arrow turn the view right (negative yaw).
    // (Q/E used to turn too; E is now the talk key.)
    input.yawDelta = -mouse.x * render::kMouseSensitivity + axis(KEY_RIGHT, KEY_LEFT) * keyTurn;

    // Mouse down / Down arrow tilt the view further down (positive pitch).
    input.pitchDelta = mouse.y * render::kMouseSensitivity + axis(KEY_UP, KEY_DOWN) * keyTurn;

    return input;
}

bool toggleMouseCapturePressed() { return IsKeyPressed(KEY_TAB); }

bool interactPressed() { return IsKeyPressed(KEY_E); }

bool keyboardOrMouseUsed(bool mouseCaptured) {
    for (const int key : {KEY_W, KEY_A, KEY_S, KEY_D, KEY_E, KEY_TAB, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN}) {
        if (IsKeyDown(key)) {
            return true;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        return true;
    }
    const Vector2 mouse = GetMouseDelta();
    return mouseCaptured && (mouse.x != 0.0f || mouse.y != 0.0f);
}

} // namespace platform
