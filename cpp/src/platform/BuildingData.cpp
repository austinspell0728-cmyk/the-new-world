// BuildingData.cpp
//
// Same line-based format and error style as game/NpcData.cpp: sections in
// square brackets, then `key = value` lines.

#include "BuildingData.h"

#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

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

// Keys each section accepts, and which of them are required.
const std::vector<std::string> kStyleKeys = {"name", "wall", "window", "door", "door_leaf",
                                             "corner", "roof", "roof_fits", "scale"};
const std::vector<std::string> kStyleRequired = {"name", "wall", "window", "door",
                                                 "door_leaf", "corner", "roof", "roof_fits"};
const std::vector<std::string> kBuildingKeys = {"tile", "style", "facing", "offset"};
const std::vector<std::string> kBuildingRequired = {"tile", "style", "facing"};

std::string listOf(const std::vector<std::string>& keys) {
    std::string text;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        text += (i == 0 ? "" : (i + 1 == keys.size() ? " or " : ", ")) + keys[i];
    }
    return text;
}

class Parser {
public:
    explicit Parser(std::string sourceName) : source_(std::move(sourceName)) {
        data_.source = source_;
    }

    BuildingData parse(std::istream& in) {
        std::string raw;
        while (std::getline(in, raw)) {
            ++lineNumber_;
            const std::string text = trim(raw);

            if (text.empty() || text.front() == '#') {
                continue; // blank line or comment
            }
            if (text == "[style]" || text == "[building]") {
                finishSection();
                section_ = (text == "[style]") ? Section::Style : Section::Building;
                sectionLine_ = lineNumber_;
                keysSeen_.clear();
                style_ = BuildingStyle{};
                building_ = BuildingEntry{};
                continue;
            }
            if (text.front() == '[') {
                fail("unknown section '" + text + "' (expected [style] or [building])");
            }

            const auto equals = text.find('=');
            if (equals == std::string::npos) {
                fail("expected '[style]', '[building]' or 'key = value', got '" + text + "'");
            }
            if (section_ == Section::None) {
                fail("'" + text + "' must come after a [style] or [building] line");
            }
            readField(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
        }
        finishSection();
        checkStyleNames();
        return data_;
    }

private:
    enum class Section { None, Style, Building };

    [[noreturn]] void fail(const std::string& message, int line = 0) const {
        throw std::runtime_error(source_ + ":" + std::to_string(line ? line : lineNumber_) + ": " +
                                 message);
    }

    std::string sectionName() const { return section_ == Section::Style ? "[style]" : "[building]"; }

    void readField(const std::string& key, const std::string& value) {
        const std::vector<std::string>& allowed =
            (section_ == Section::Style) ? kStyleKeys : kBuildingKeys;
        bool known = false;
        for (const std::string& k : allowed) {
            known = known || (k == key);
        }
        if (!known) {
            fail("unknown key '" + key + "' in " + sectionName() + " (expected " + listOf(allowed) +
                 ")");
        }
        if (!keysSeen_.insert(key).second) {
            fail("'" + key + "' is set twice in the same " + sectionName());
        }
        if (value.empty()) {
            fail("'" + key + "' has no value");
        }

        if (section_ == Section::Style) {
            readStyleField(key, value);
        } else {
            readBuildingField(key, value);
        }
    }

    void readStyleField(const std::string& key, const std::string& value) {
        if (key == "name") {
            style_.name = value;
        } else if (key == "wall") {
            style_.wall = value;
        } else if (key == "window") {
            style_.window = value;
        } else if (key == "door") {
            style_.door = value;
        } else if (key == "door_leaf") {
            style_.doorLeaf = value;
        } else if (key == "corner") {
            style_.corner = value;
        } else if (key == "roof") {
            style_.roof = value;
        } else if (key == "roof_fits") {
            const std::vector<float> n = parseNumbers(value, 2, "roof_fits", "two whole numbers like '6, 8'");
            if (n[0] != std::floor(n[0]) || n[1] != std::floor(n[1]) || n[0] < 1 || n[1] < 1) {
                fail("roof_fits must be two whole numbers of at least 1, got '" + value + "'");
            }
            style_.roofFitsX = static_cast<int>(n[0]);
            style_.roofFitsZ = static_cast<int>(n[1]);
        } else if (key == "scale") {
            const float scale = parseNumbers(value, 1, "scale", "a number")[0];
            if (scale <= 0.0f) {
                fail("scale must be more than 0, got '" + value + "'");
            }
            style_.heightScale = scale;
        }
    }

    void readBuildingField(const std::string& key, const std::string& value) {
        if (key == "tile") {
            const std::vector<float> n = parseNumbers(value, 2, "tile", "two whole numbers like '29, 24'");
            if (n[0] != std::floor(n[0]) || n[1] != std::floor(n[1])) {
                fail("tile must be two whole numbers like '29, 24', got '" + value + "'");
            }
            building_.tileX = static_cast<int>(n[0]);
            building_.tileZ = static_cast<int>(n[1]);
        } else if (key == "style") {
            building_.style = value;
            styleLines_[data_.buildings.size()] = lineNumber_;
        } else if (key == "facing") {
            const float facing = parseNumbers(value, 1, "facing", "0, 90, 180 or 270")[0];
            if (facing != 0.0f && facing != 90.0f && facing != 180.0f && facing != 270.0f) {
                fail("facing must be 0, 90, 180 or 270, got '" + value + "'");
            }
            building_.facingDegrees = static_cast<int>(facing);
        } else if (key == "offset") {
            const std::vector<float> n = parseNumbers(value, 3, "offset", "three numbers like '0, 0.1, 0'");
            building_.offsetX = n[0];
            building_.offsetY = n[1];
            building_.offsetZ = n[2];
        }
    }

    // Reads exactly `count` comma-separated numbers.
    std::vector<float> parseNumbers(const std::string& value, std::size_t count, const std::string& key,
                                    const std::string& expected) const {
        std::vector<float> numbers;
        std::istringstream stream(value);
        std::string part;
        while (std::getline(stream, part, ',')) {
            std::istringstream partStream(trim(part));
            float number = 0.0f;
            std::string rest;
            if (!(partStream >> number) || (partStream >> rest) || !std::isfinite(number)) {
                fail(key + " must be " + expected + ", got '" + value + "'");
            }
            numbers.push_back(number);
        }
        if (numbers.size() != count || value.back() == ',') {
            fail(key + " must be " + expected + ", got '" + value + "'");
        }
        return numbers;
    }

    // Checks the finished section has every required key, then stores it.
    void finishSection() {
        if (section_ == Section::None) {
            return;
        }
        const std::vector<std::string>& required =
            (section_ == Section::Style) ? kStyleRequired : kBuildingRequired;
        for (const std::string& key : required) {
            if (keysSeen_.count(key) == 0) {
                fail(sectionName() + " is missing '" + key + "'", sectionLine_);
            }
        }

        if (section_ == Section::Style) {
            if (data_.findStyle(style_.name) != nullptr) {
                fail("style '" + style_.name + "' is defined twice", sectionLine_);
            }
            style_.line = sectionLine_;
            data_.styles.push_back(style_);
        } else {
            for (const BuildingEntry& other : data_.buildings) {
                if (other.tileX == building_.tileX && other.tileZ == building_.tileZ) {
                    fail("tile " + std::to_string(building_.tileX) + ", " +
                             std::to_string(building_.tileZ) + " is already used by the [building] on line " +
                             std::to_string(other.line),
                         sectionLine_);
                }
            }
            building_.line = sectionLine_;
            data_.buildings.push_back(building_);
        }
        section_ = Section::None;
    }

    // Styles may be defined before or after the buildings that use them, so
    // names are checked once the whole file has been read.
    void checkStyleNames() const {
        for (std::size_t i = 0; i < data_.buildings.size(); ++i) {
            const BuildingEntry& b = data_.buildings[i];
            if (data_.findStyle(b.style) == nullptr) {
                fail("unknown style '" + b.style + "'", styleLines_.at(i));
            }
        }
    }

    std::string source_;
    int lineNumber_ = 0;
    Section section_ = Section::None;
    int sectionLine_ = 0;
    std::set<std::string> keysSeen_;
    BuildingStyle style_;
    BuildingEntry building_;
    std::map<std::size_t, int> styleLines_; // building index -> line of its style key
    BuildingData data_;
};

} // namespace

const BuildingStyle* BuildingData::findStyle(const std::string& name) const {
    for (const BuildingStyle& style : styles) {
        if (style.name == name) {
            return &style;
        }
    }
    return nullptr;
}

BuildingData parseBuildings(std::istream& in, const std::string& sourceName) {
    return Parser(sourceName).parse(in);
}

BuildingData loadBuildingFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open building file '" + path + "'.");
    }
    return parseBuildings(file, path);
}

} // namespace platform
