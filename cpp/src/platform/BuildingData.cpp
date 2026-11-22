// BuildingData.cpp
//
// Same line-based format and error style as game/NpcData.cpp: sections in
// square brackets, then `key = value` lines.

#include "BuildingData.h"

#include <cmath>
#include <fstream>
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

enum class Section { None, Style, Roof, Building };

// Keys each section accepts, and which of them are required.
struct SectionKeys {
    const char* header;
    std::vector<std::string> allowed;
    std::vector<std::string> required;
};

const SectionKeys& keysFor(Section section) {
    static const SectionKeys style{"[style]",
                                   {"name", "wall", "window", "window_frame", "door", "door_leaf", "corner", "scale"},
                                   {"name", "wall", "window", "window_frame", "door", "door_leaf", "corner"}};
    static const SectionKeys roof{"[roof]", {"name", "model", "gable", "fits"}, {"name", "model", "gable", "fits"}};
    static const SectionKeys building{"[building]",
                                      {"tile", "style", "facing", "offset", "roof"},
                                      {"tile", "style", "facing"}};
    return section == Section::Style ? style : section == Section::Roof ? roof : building;
}

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
            if (text == "[style]" || text == "[roof]" || text == "[building]") {
                finishSection();
                section_ = text == "[style]" ? Section::Style : text == "[roof]" ? Section::Roof : Section::Building;
                sectionLine_ = lineNumber_;
                keysSeen_.clear();
                style_ = BuildingStyle{};
                roof_ = BuildingRoof{};
                building_ = BuildingEntry{};
                continue;
            }
            if (text.front() == '[') {
                fail("unknown section '" + text + "' (expected [style], [roof] or [building])");
            }

            const auto equals = text.find('=');
            if (equals == std::string::npos) {
                fail("expected a [section] or 'key = value', got '" + text + "'");
            }
            if (section_ == Section::None) {
                fail("'" + text + "' must come after a [style], [roof] or [building] line");
            }
            readField(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
        }
        finishSection();
        checkNames();
        return data_;
    }

private:
    [[noreturn]] void fail(const std::string& message, int line = 0) const {
        throw std::runtime_error(source_ + ":" + std::to_string(line ? line : lineNumber_) + ": " +
                                 message);
    }

    void readField(const std::string& key, const std::string& value) {
        const SectionKeys& keys = keysFor(section_);
        bool known = false;
        for (const std::string& k : keys.allowed) {
            known = known || (k == key);
        }
        if (!known) {
            fail("unknown key '" + key + "' in " + keys.header + " (expected " + listOf(keys.allowed) + ")");
        }
        if (!keysSeen_.insert(key).second) {
            fail("'" + key + "' is set twice in the same " + keys.header);
        }
        if (value.empty()) {
            fail("'" + key + "' has no value");
        }

        switch (section_) {
        case Section::Style: readStyleField(key, value); break;
        case Section::Roof: readRoofField(key, value); break;
        case Section::Building: readBuildingField(key, value); break;
        case Section::None: break;
        }
    }

    void readStyleField(const std::string& key, const std::string& value) {
        if (key == "name") {
            style_.name = value;
        } else if (key == "wall") {
            style_.wall = value;
        } else if (key == "window") {
            style_.window = value;
        } else if (key == "window_frame") {
            style_.windowFrame = value;
        } else if (key == "door") {
            style_.door = value;
        } else if (key == "door_leaf") {
            style_.doorLeaf = value;
        } else if (key == "corner") {
            style_.corner = value;
        } else if (key == "scale") {
            const float scale = parseNumbers(value, 1, "scale", "a number")[0];
            if (scale <= 0.0f) {
                fail("scale must be more than 0, got '" + value + "'");
            }
            style_.heightScale = scale;
        }
    }

    void readRoofField(const std::string& key, const std::string& value) {
        if (key == "name") {
            roof_.name = value;
        } else if (key == "model") {
            roof_.model = value;
        } else if (key == "gable") {
            roof_.gable = value;
        } else if (key == "fits") {
            const std::vector<float> n = parseNumbers(value, 2, "fits", "two whole numbers like '6, 8'");
            if (n[0] != std::floor(n[0]) || n[1] != std::floor(n[1]) || n[0] < 1 || n[1] < 1) {
                fail("fits must be two whole numbers of at least 1, got '" + value + "'");
            }
            roof_.fitsAcross = static_cast<int>(n[0]);
            roof_.fitsAlong = static_cast<int>(n[1]);
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
            pendingNames_.push_back({"style", value, lineNumber_});
        } else if (key == "roof") {
            building_.roof = value;
            pendingNames_.push_back({"roof", value, lineNumber_});
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
        const SectionKeys& keys = keysFor(section_);
        for (const std::string& key : keys.required) {
            if (keysSeen_.count(key) == 0) {
                fail(std::string(keys.header) + " is missing '" + key + "'", sectionLine_);
            }
        }

        if (section_ == Section::Style) {
            if (data_.findStyle(style_.name) != nullptr) {
                fail("style '" + style_.name + "' is defined twice", sectionLine_);
            }
            style_.line = sectionLine_;
            data_.styles.push_back(style_);
        } else if (section_ == Section::Roof) {
            if (data_.findRoof(roof_.name) != nullptr) {
                fail("roof '" + roof_.name + "' is defined twice", sectionLine_);
            }
            roof_.line = sectionLine_;
            data_.roofs.push_back(roof_);
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

    // Styles and roofs may be defined before or after the buildings that
    // use them, so names are checked once the whole file has been read.
    void checkNames() const {
        for (const PendingName& name : pendingNames_) {
            const bool found = (name.kind == "style") ? data_.findStyle(name.value) != nullptr
                                                      : data_.findRoof(name.value) != nullptr;
            if (!found) {
                fail("unknown " + name.kind + " '" + name.value + "'", name.line);
            }
        }
        if (!data_.buildings.empty() && data_.roofs.empty()) {
            fail("there are buildings but no [roof] for them", data_.buildings.front().line);
        }
    }

    struct PendingName {
        std::string kind;  // "style" or "roof"
        std::string value;
        int line;
    };

    std::string source_;
    int lineNumber_ = 0;
    Section section_ = Section::None;
    int sectionLine_ = 0;
    std::set<std::string> keysSeen_;
    BuildingStyle style_;
    BuildingRoof roof_;
    BuildingEntry building_;
    std::vector<PendingName> pendingNames_;
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

const BuildingRoof* BuildingData::findRoof(const std::string& name) const {
    for (const BuildingRoof& roof : roofs) {
        if (roof.name == name) {
            return &roof;
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
