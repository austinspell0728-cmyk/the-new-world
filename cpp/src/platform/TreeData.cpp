// TreeData.cpp
//
// Same line-based format as BuildingData.cpp: sections in square brackets,
// then `key = value` lines.

#include "TreeData.h"

#include <cmath>
#include <fstream>
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

class Parser {
public:
    explicit Parser(std::string sourceName) : source_(std::move(sourceName)) {
        data_.source = source_;
    }

    TreeData parse(std::istream& in) {
        std::string raw;
        while (std::getline(in, raw)) {
            ++lineNumber_;
            const std::string text = trim(raw);

            if (text.empty() || text.front() == '#') {
                continue; // blank line or comment
            }
            if (text == "[tree]" || text == "[rules]") {
                finishSection();
                section_ = (text == "[tree]") ? Section::Tree : Section::Rules;
                if (section_ == Section::Rules && rulesLine_ != 0) {
                    fail("[rules] appears twice (first on line " + std::to_string(rulesLine_) + ")");
                }
                sectionLine_ = lineNumber_;
                if (section_ == Section::Rules) {
                    rulesLine_ = lineNumber_;
                }
                keysSeen_.clear();
                tree_ = TreeVariant{};
                continue;
            }
            if (text.front() == '[') {
                fail("unknown section '" + text + "' (expected [tree] or [rules])");
            }

            const auto equals = text.find('=');
            if (equals == std::string::npos) {
                fail("expected a [section] or 'key = value', got '" + text + "'");
            }
            if (section_ == Section::None) {
                fail("'" + text + "' must come after a [tree] or [rules] line");
            }
            readField(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
        }
        finishSection();
        return data_;
    }

private:
    enum class Section { None, Tree, Rules };

    [[noreturn]] void fail(const std::string& message, int line = 0) const {
        throw std::runtime_error(source_ + ":" + std::to_string(line ? line : lineNumber_) + ": " +
                                 message);
    }

    void readField(const std::string& key, const std::string& value) {
        const bool tree = (section_ == Section::Tree);
        const bool known = tree ? (key == "name" || key == "model" || key == "scale" || key == "trunk" ||
                                   key == "weight")
                                : (key == "scale_variation" || key == "seed");
        if (!known) {
            fail("unknown key '" + key + "' in " +
                 (tree ? "[tree] (expected name, model, scale, trunk or weight)"
                       : "[rules] (expected scale_variation or seed)"));
        }
        if (!keysSeen_.insert(key).second) {
            fail("'" + key + "' is set twice in the same " + (tree ? "[tree]" : "[rules]"));
        }
        if (value.empty()) {
            fail("'" + key + "' has no value");
        }

        if (key == "name") {
            tree_.name = value;
        } else if (key == "model") {
            tree_.model = value;
        } else if (key == "scale") {
            tree_.scale = parseNumbers(value, 1, "scale", "a number")[0];
            if (tree_.scale <= 0.0f) {
                fail("scale must be more than 0, got '" + value + "'");
            }
        } else if (key == "trunk") {
            const std::vector<float> n = parseNumbers(value, 2, "trunk", "two numbers like '0, 0.2'");
            tree_.trunkX = n[0];
            tree_.trunkZ = n[1];
        } else if (key == "weight") {
            const float weight = parseNumbers(value, 1, "weight", "a whole number of at least 1")[0];
            if (weight != std::floor(weight) || weight < 1.0f || weight > 1000.0f) {
                fail("weight must be a whole number from 1 to 1000, got '" + value + "'");
            }
            tree_.weight = static_cast<int>(weight);
        } else if (key == "scale_variation") {
            const float variation = parseNumbers(value, 1, "scale_variation", "a number from 0 to 0.5")[0];
            if (variation < 0.0f || variation > 0.5f) {
                fail("scale_variation must be a number from 0 to 0.5, got '" + value + "'");
            }
            data_.rules.scaleVariation = variation;
        } else if (key == "seed") {
            const float seed = parseNumbers(value, 1, "seed", "a whole number from 0 to 1000000")[0];
            if (seed != std::floor(seed) || seed < 0.0f || seed > 1000000.0f) {
                fail("seed must be a whole number from 0 to 1000000, got '" + value + "'");
            }
            data_.rules.seed = static_cast<std::uint32_t>(seed);
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

    // Checks a finished [tree] has every required key, then stores it.
    void finishSection() {
        if (section_ == Section::Tree) {
            for (const char* key : {"name", "model", "scale", "trunk"}) {
                if (keysSeen_.count(key) == 0) {
                    fail(std::string("[tree] is missing '") + key + "'", sectionLine_);
                }
            }
            if (data_.findVariant(tree_.name) != nullptr) {
                fail("tree '" + tree_.name + "' is defined twice", sectionLine_);
            }
            tree_.line = sectionLine_;
            data_.variants.push_back(tree_);
        }
        section_ = Section::None;
    }

    std::string source_;
    int lineNumber_ = 0;
    Section section_ = Section::None;
    int sectionLine_ = 0;
    int rulesLine_ = 0; // line of the [rules] section, 0 = none yet
    std::set<std::string> keysSeen_;
    TreeVariant tree_;
    TreeData data_;
};

} // namespace

const TreeVariant* TreeData::findVariant(const std::string& name) const {
    for (const TreeVariant& variant : variants) {
        if (variant.name == name) {
            return &variant;
        }
    }
    return nullptr;
}

TreeData parseTrees(std::istream& in, const std::string& sourceName) {
    return Parser(sourceName).parse(in);
}

TreeData loadTreeFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open tree file '" + path + "'.");
    }
    return parseTrees(file, path);
}

} // namespace platform
