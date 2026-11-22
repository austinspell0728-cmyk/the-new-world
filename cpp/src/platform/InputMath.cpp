// InputMath.cpp

#include "InputMath.h"

#include <algorithm>
#include <cmath>

namespace platform {

namespace {

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

float lengthOf(Stick s) { return std::sqrt(s.x * s.x + s.y * s.y); }

} // namespace

Stick applyRadialDeadZone(Stick raw, float deadZone) {
    if (!std::isfinite(raw.x) || !std::isfinite(raw.y)) {
        return {};
    }
    const float length = lengthOf(raw);
    deadZone = std::clamp(deadZone, 0.0f, 0.99f);
    if (length <= deadZone) {
        return {};
    }
    // Cap at 1 (a pad's corner can read about 1.41), then rescale the part
    // outside the dead zone to 0..1.
    const float capped = std::min(length, 1.0f);
    const float scaled = (capped - deadZone) / (1.0f - deadZone);
    return {raw.x / length * scaled, raw.y / length * scaled};
}

float responseCurve(float amount, float exponent) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    return std::pow(amount, std::max(exponent, 1.0f));
}

Stick shapeStick(Stick raw, float deadZone, float curveExponent) {
    const Stick live = applyRadialDeadZone(raw, deadZone);
    const float length = lengthOf(live);
    if (length <= 0.0f) {
        return {};
    }
    const float curved = responseCurve(length, curveExponent);
    return {live.x / length * curved, live.y / length * curved};
}

MoveAxes stickToMove(Stick shaped) { return {shaped.y, shaped.x}; }

MoveAxes combineMove(MoveAxes keyboard, MoveAxes gamepad) {
    return {keyboard.forward + gamepad.forward, keyboard.right + gamepad.right};
}

CameraTurn stickToCameraTurn(Stick shaped, float degreesPerSecond, bool invertY, float frameTime) {
    if (!std::isfinite(frameTime) || frameTime <= 0.0f) {
        return {};
    }
    const float turn = degreesPerSecond * kDegToRad * frameTime;
    CameraTurn result;
    // Right turns the view right: negative yaw, like moving the mouse right.
    result.yawDelta = -shaped.x * turn;
    // Up tilts the view up: negative pitch (less steeply down).
    result.pitchDelta = (invertY ? shaped.y : -shaped.y) * turn;
    return result;
}

CameraTurn combineTurn(CameraTurn mouseAndKeys, CameraTurn gamepad) {
    return {mouseAndKeys.yawDelta + gamepad.yawDelta, mouseAndKeys.pitchDelta + gamepad.pitchDelta};
}

InputDevice LastUsedDevice::update(bool keyboardMouseUsed, bool gamepadUsed, bool gamepadConnected) {
    if (!gamepadConnected) {
        current_ = InputDevice::KeyboardMouse;
    } else if (keyboardMouseUsed && !gamepadUsed) {
        current_ = InputDevice::KeyboardMouse;
    } else if (gamepadUsed && !keyboardMouseUsed) {
        current_ = InputDevice::Gamepad;
    }
    return current_;
}

} // namespace platform
