// Gamepad.cpp

#include "Gamepad.h"

#include <iostream>

#include "raylib.h"

namespace platform {

namespace {

// raylib supports up to 4 pads; we use the first one that is connected.
constexpr int kMaxPads = 4;

int toRaylib(PadButton button) {
    switch (button) {
    case PadButton::A: return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    case PadButton::B: return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    case PadButton::X: return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    case PadButton::Y: return GAMEPAD_BUTTON_RIGHT_FACE_UP;
    case PadButton::LB: return GAMEPAD_BUTTON_LEFT_TRIGGER_1;
    case PadButton::RB: return GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
    case PadButton::LT: return GAMEPAD_BUTTON_LEFT_TRIGGER_2;
    case PadButton::RT: return GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
    case PadButton::Back: return GAMEPAD_BUTTON_MIDDLE_LEFT;
    case PadButton::Start: return GAMEPAD_BUTTON_MIDDLE_RIGHT;
    case PadButton::LS: return GAMEPAD_BUTTON_LEFT_THUMB;
    case PadButton::RS: return GAMEPAD_BUTTON_RIGHT_THUMB;
    case PadButton::DpadUp: return GAMEPAD_BUTTON_LEFT_FACE_UP;
    case PadButton::DpadDown: return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    case PadButton::DpadLeft: return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    case PadButton::DpadRight: return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    case PadButton::None: break;
    }
    return GAMEPAD_BUTTON_UNKNOWN;
}

bool pressed(int pad, PadButton button) {
    return button != PadButton::None && IsGamepadButtonPressed(pad, toRaylib(button));
}

// A stick's raw reading, flipped to y = up (raylib reports up as -1).
Stick rawStick(int pad, PadStick which) {
    const int xAxis = (which == PadStick::Left) ? GAMEPAD_AXIS_LEFT_X : GAMEPAD_AXIS_RIGHT_X;
    const int yAxis = (which == PadStick::Left) ? GAMEPAD_AXIS_LEFT_Y : GAMEPAD_AXIS_RIGHT_Y;
    return {GetGamepadAxisMovement(pad, xAxis), -GetGamepadAxisMovement(pad, yAxis)};
}

bool live(Stick s) { return s.x != 0.0f || s.y != 0.0f; }

} // namespace

GamepadReader::GamepadReader(const ControlsData& controls, bool enabled, std::optional<FakePad> fake)
    : controls_(controls), enabled_(enabled), fake_(fake) {}

GamepadFrame GamepadReader::read() {
    GamepadFrame frame;

    if (fake_) {
        frame.connected = true;
        frame.name = "scripted test pad (--fake-pad)";
        frame.move = shapeStick(fake_->move, controls_.deadZone, controls_.responseCurve);
        frame.camera = shapeStick(fake_->camera, controls_.deadZone, controls_.responseCurve);
        frame.used = live(frame.move) || live(frame.camera);
        if (!announced_) {
            std::cout << "Input: using the scripted test pad.\n" << std::flush;
            announced_ = true;
        }
        return frame;
    }
    if (!enabled_) {
        return frame;
    }

    // Find the first connected pad (it may have been plugged in or out
    // since the last frame).
    int found = -1;
    for (int i = 0; i < kMaxPads && found < 0; ++i) {
        if (IsGamepadAvailable(i)) {
            found = i;
        }
    }
    if (found >= 0) {
        const char* name = GetGamepadName(found);
        frame.name = (name != nullptr && name[0] != '\0') ? name : "unnamed gamepad";
    }

    // Say so once at startup, then only when a pad comes or goes.
    if (!announced_) {
        std::cout << (found >= 0 ? "Input: gamepad connected: " + frame.name + ".\n"
                                 : std::string("Input: no gamepad found; keyboard and mouse only "
                                               "(a pad can be plugged in at any time).\n"))
                  << std::flush;
        announced_ = true;
    } else if (found != padIndex_) {
        std::cout << (found >= 0 ? "Input: gamepad connected: " + frame.name + ".\n"
                                 : std::string("Input: gamepad disconnected.\n"))
                  << std::flush;
    }
    padIndex_ = found;
    if (found < 0) {
        return frame;
    }

    frame.connected = true;
    frame.move = shapeStick(rawStick(found, controls_.moveStick), controls_.deadZone, controls_.responseCurve);
    frame.camera = shapeStick(rawStick(found, controls_.cameraStick), controls_.deadZone, controls_.responseCurve);
    frame.interactPressed = pressed(found, controls_.interact);
    frame.pausePressed = pressed(found, controls_.pause);
    // Any button counts as using the pad (for the hints), not only mapped ones.
    frame.used = live(frame.move) || live(frame.camera) || GetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN;
    return frame;
}

} // namespace platform
