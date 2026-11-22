// NpcData.cpp

#include "NpcData.h"

#include <cmath>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

#include "GameData.h"

namespace game {

namespace {

constexpr float kPi = 3.14159265358979f;

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

// Fields of one [npc] block while it is being read.
struct PendingNpc {
    int startLine = 0;
    std::optional<std::string> name;
    std::optional<std::string> line;
    std::optional<std::uint32_t> color;
    std::optional<std::pair<int, int>> tile;
    float facingDegrees = 0.0f;
    std::set<std::string> keysSeen;
};

class Parser {
public:
    explicit Parser(std::string sourceName) : source_(std::move(sourceName)) {}

    std::vector<Npc> parse(std::istream& in) {
        std::string raw;
        while (std::getline(in, raw)) {
            ++lineNumber_;
            const std::string text = trim(raw);

            if (text.empty() || text.front() == '#') {
                continue; // blank line or comment
            }
            if (text == "[npc]") {
                finishPending();
                pending_ = PendingNpc{};
                pending_->startLine = lineNumber_;
                continue;
            }

            const auto equals = text.find('=');
            if (equals == std::string::npos) {
                fail("expected '[npc]' or 'key = value', got '" + text + "'");
            }
            if (!pending_) {
                fail("'" + text + "' must come after an [npc] line");
            }
            readField(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
        }
        finishPending();
        return npcs_;
    }

private:
    [[noreturn]] void fail(const std::string& message, int line = 0) const {
        throw std::runtime_error(source_ + ":" + std::to_string(line ? line : lineNumber_) + ": " +
                                 message);
    }

    void readField(const std::string& key, const std::string& value) {
        if (!pending_->keysSeen.insert(key).second) {
            fail("'" + key + "' is set twice in the same [npc]");
        }
        if (value.empty()) {
            fail("'" + key + "' has no value");
        }

        if (key == "name") {
            pending_->name = value;
        } else if (key == "line") {
            pending_->line = value;
        } else if (key == "color") {
            pending_->color = parseColor(value);
        } else if (key == "tile") {
            pending_->tile = parseTile(value);
        } else if (key == "facing") {
            pending_->facingDegrees = parseNumber(value, "facing");
        } else {
            fail("unknown key '" + key + "' (expected name, tile, facing, color or line)");
        }
    }

    // "#RRGGBB" -> 0xRRGGBB
    std::uint32_t parseColor(const std::string& value) const {
        const bool hex = value.size() == 7 && value[0] == '#' &&
                         value.find_first_not_of("0123456789abcdefABCDEF", 1) == std::string::npos;
        if (!hex) {
            fail("color must look like #RRGGBB, got '" + value + "'");
        }
        return static_cast<std::uint32_t>(std::stoul(value.substr(1), nullptr, 16));
    }

    // "x, z" -> two whole numbers
    std::pair<int, int> parseTile(const std::string& value) const {
        std::istringstream stream(value);
        int x = 0, z = 0;
        char comma = 0;
        std::string rest;
        if (!(stream >> x >> comma >> z) || comma != ',' || (stream >> rest)) {
            fail("tile must be two whole numbers like '20, 13', got '" + value + "'");
        }
        return {x, z};
    }

    float parseNumber(const std::string& value, const std::string& key) const {
        std::istringstream stream(value);
        float number = 0.0f;
        std::string rest;
        if (!(stream >> number) || (stream >> rest) || !std::isfinite(number)) {
            fail(key + " must be a number, got '" + value + "'");
        }
        return number;
    }

    // Turns the finished block into an Npc, checking nothing required is missing.
    void finishPending() {
        if (!pending_) {
            return;
        }
        const PendingNpc& p = *pending_;
        for (const char* required : {"name", "tile", "color", "line"}) {
            if (p.keysSeen.count(required) == 0) {
                fail(std::string("[npc] is missing '") + required + "'", p.startLine);
            }
        }

        Npc npc;
        npc.name = *p.name;
        npc.line = *p.line;
        npc.color = *p.color;
        // Stand in the centre of the tile, on the ground.
        npc.position = {static_cast<float>(p.tile->first) + 0.5f, 0.0f,
                        static_cast<float>(p.tile->second) + 0.5f};
        npc.facingYaw = p.facingDegrees * kPi / 180.0f;
        npc.halfSize = data::kNpcHalfSize;
        npc.height = data::kNpcHeight;
        npcs_.push_back(std::move(npc));
        pending_.reset();
    }

    std::string source_;
    int lineNumber_ = 0;
    std::optional<PendingNpc> pending_;
    std::vector<Npc> npcs_;
};

} // namespace

std::vector<Npc> parseNpcs(std::istream& in, const std::string& sourceName) {
    return Parser(sourceName).parse(in);
}

std::vector<Npc> loadNpcFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open NPC file '" + path + "'.");
    }
    return parseNpcs(file, path);
}

} // namespace game
