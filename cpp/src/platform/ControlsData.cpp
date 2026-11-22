// ControlsData.cpp
//
// Same line-based format as the other data files: sections in square
// brackets, then `key = value` lines.

#include "ControlsData.h"

#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace platform {

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

// Button names in controls.txt, with the label shown on screen.
struct ButtonName {
    const char* name;
    const char* label;
    PadButton button;
};
constexpr std::array<ButtonName, 17> kButtons = {{
    {"none", "-", PadButton::None},
    {"a", "A", PadButton::A},
    {"b", "B", PadButton::B},
    {"x", "X", PadButton::X},
    {"y", "Y", PadButton::Y},
    {"lb", "LB", PadButton::LB},
    {"rb", "RB", PadButton::RB},
    {"lt", "LT", PadButton::LT},
    {"rt", "RT", PadButton::RT},
    {"back", "Back", PadButton::Back},
    {"start", "Start", PadButton::Start},
    {"ls", "LS", PadButton::LS},
    {"rs", "RS", PadButton::RS},
    {"dpad_up", "D-pad up", PadButton::DpadUp},
    {"dpad_down", "D-pad down", PadButton::DpadDown},
    {"dpad_left", "D-pad left", PadButton::DpadLeft},
    {"dpad_right", "D-pad right", PadButton::DpadRight},
}};

const char* kGamepadKeys[] = {"move_stick", "camera_stick", "dead_zone", "response_curve", "camera_speed", "invert_y"};
const char* kButtonKeys[] = {"interact", "pause", "attack", "dodge", "cast"};

class Parser {
public:
    explicit Parser(std::string sourceName) : source_(std::move(sourceName)) { data_.source = source_; }

    ControlsData parse(std::istream& in) {
        std::string raw;
        while (std::getline(in, raw)) {
            ++lineNumber_;
            const std::string text = trim(raw);
            if (text.empty() || text.front() == '#') {
                continue;
            }
            if (text == "[gamepad]" || text == "[buttons]") {
                finishSection();
                section_ = (text == "[gamepad]") ? Section::Gamepad : Section::Buttons;
                int& seenOn = (section_ == Section::Gamepad) ? gamepadLine_ : buttonsLine_;
                if (seenOn != 0) {
                    fail(text + " appears twice (first on line " + std::to_string(seenOn) + ")");
                }
                seenOn = lineNumber_;
                keysSeen_.clear();
                continue;
            }
            if (text.front() == '[') {
                fail("unknown section '" + text + "' (expected [gamepad] or [buttons])");
            }
            const auto equals = text.find('=');
            if (equals == std::string::npos) {
                fail("expected a [section] or 'key = value', got '" + text + "'");
            }
            if (section_ == Section::None) {
                fail("'" + text + "' must come after a [gamepad] or [buttons] line");
            }
            readField(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
        }
        finishSection();
        if (gamepadLine_ == 0) {
            fail("the file has no [gamepad] section", 1);
        }
        if (buttonsLine_ == 0) {
            fail("the file has no [buttons] section", 1);
        }
        return data_;
    }

private:
    enum class Section { None, Gamepad, Buttons };

    [[noreturn]] void fail(const std::string& message, int line = 0) const {
        throw std::runtime_error(source_ + ":" + std::to_string(line ? line : lineNumber_) + ": " + message);
    }

    void readField(const std::string& key, const std::string& value) {
        const bool gamepad = (section_ == Section::Gamepad);
        bool known = false;
        if (gamepad) {
            for (const char* k : kGamepadKeys) known = known || key == k;
        } else {
            for (const char* k : kButtonKeys) known = known || key == k;
        }
        if (!known) {
            fail("unknown key '" + key + "' in " +
                 (gamepad ? "[gamepad] (expected move_stick, camera_stick, dead_zone, response_curve, "
                            "camera_speed or invert_y)"
                          : "[buttons] (expected interact, pause, attack, dodge or cast)"));
        }
        if (!keysSeen_.insert(key).second) {
            fail("'" + key + "' is set twice in the same " + (gamepad ? "[gamepad]" : "[buttons]"));
        }
        if (value.empty()) {
            fail("'" + key + "' has no value");
        }
        if (gamepad) {
            readGamepadField(key, value);
        } else {
            readButtonField(key, value);
        }
    }

    void readGamepadField(const std::string& key, const std::string& value) {
        if (key == "move_stick" || key == "camera_stick") {
            if (value != "left" && value != "right") {
                fail(key + " must be left or right, got '" + value + "'");
            }
            (key == "move_stick" ? data_.moveStick : data_.cameraStick) =
                (value == "left") ? PadStick::Left : PadStick::Right;
            stickLines_[key] = lineNumber_;
        } else if (key == "dead_zone") {
            data_.deadZone = numberIn(value, key, 0.0f, 0.9f, "0 to 0.9");
        } else if (key == "response_curve") {
            data_.responseCurve = numberIn(value, key, 1.0f, 3.0f, "1 to 3");
        } else if (key == "camera_speed") {
            data_.cameraSpeed = numberIn(value, key, 10.0f, 720.0f, "10 to 720");
        } else if (key == "invert_y") {
            if (value != "yes" && value != "no") {
                fail("invert_y must be yes or no, got '" + value + "'");
            }
            data_.invertY = (value == "yes");
        }
    }

    void readButtonField(const std::string& action, const std::string& value) {
        const ButtonName* found = nullptr;
        for (const ButtonName& b : kButtons) {
            if (value == b.name) {
                found = &b;
            }
        }
        if (found == nullptr) {
            fail("unknown button '" + value +
                 "' (expected a, b, x, y, lb, rb, lt, rt, back, start, ls, rs, dpad_up, dpad_down, "
                 "dpad_left, dpad_right or none)");
        }
        if (found->button != PadButton::None) {
            const auto used = buttonUsers_.find(found->button);
            if (used != buttonUsers_.end()) {
                fail("button '" + value + "' is already used for '" + used->second.first + "' on line " +
                     std::to_string(used->second.second));
            }
            buttonUsers_[found->button] = {action, lineNumber_};
        }
        PadButton& slot = action == "interact" ? data_.interact
                          : action == "pause"  ? data_.pause
                          : action == "attack" ? data_.attack
                          : action == "dodge"  ? data_.dodge
                                               : data_.cast;
        slot = found->button;
    }

    // A number from `low` to `high` (`range` says so in the error).
    float numberIn(const std::string& value, const std::string& key, float low, float high,
                   const std::string& range) const {
        std::istringstream stream(value);
        float number = 0.0f;
        std::string rest;
        if (!(stream >> number) || (stream >> rest) || !std::isfinite(number) || number < low || number > high) {
            fail(key + " must be a number from " + range + ", got '" + value + "'");
        }
        return number;
    }

    // Checks a finished section has every required key.
    void finishSection() {
        if (section_ == Section::Gamepad) {
            for (const char* key : kGamepadKeys) {
                if (keysSeen_.count(key) == 0) {
                    fail(std::string("[gamepad] is missing '") + key + "'", gamepadLine_);
                }
            }
            if (data_.moveStick == data_.cameraStick) {
                fail("move_stick and camera_stick are both the " +
                         std::string(data_.moveStick == PadStick::Left ? "left" : "right") + " stick",
                     stickLines_["camera_stick"]);
            }
        } else if (section_ == Section::Buttons) {
            for (const char* key : {"interact", "pause"}) {
                if (keysSeen_.count(key) == 0) {
                    fail(std::string("[buttons] is missing '") + key + "'", buttonsLine_);
                }
            }
        }
        section_ = Section::None;
    }

    std::string source_;
    int lineNumber_ = 0;
    Section section_ = Section::None;
    int gamepadLine_ = 0;
    int buttonsLine_ = 0;
    std::set<std::string> keysSeen_;
    std::map<std::string, int> stickLines_;
    std::map<PadButton, std::pair<std::string, int>> buttonUsers_; // button -> action, line
    ControlsData data_;
};

} // namespace

const char* buttonLabel(PadButton button) {
    for (const ButtonName& b : kButtons) {
        if (b.button == button) {
            return b.label;
        }
    }
    return "?";
}

ControlsData parseControls(std::istream& in, const std::string& sourceName) {
    return Parser(sourceName).parse(in);
}

ControlsData loadControlsFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open controls file '" + path + "'.");
    }
    return parseControls(file, path);
}

} // namespace platform
