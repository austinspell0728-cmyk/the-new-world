// WorldBuilder.cpp

#include "WorldBuilder.h"

#include <stdexcept>

#include "GameData.h"

namespace game {

MapRows generateTerrain(std::uint32_t seed, int width, int depth) {
    if (width < 3 || depth < 3) {
        throw std::invalid_argument("Terrain must be at least 3x3 tiles.");
    }

    // ------------------------------------------------------------------
    // SEEDED TERRAIN GENERATION GOES HERE.
    // For now: flat grass with a stone wall around the edge, the same for
    // every seed. A generator should fill `rows` from `seed` instead, and
    // keep the border wall (or another way to stop the player leaving).
    // ------------------------------------------------------------------
    static_cast<void>(seed); // unused until generation exists

    MapRows rows(static_cast<std::size_t>(depth), std::string(static_cast<std::size_t>(width), '.'));
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            const bool edge = (x == 0 || z == 0 || x == width - 1 || z == depth - 1);
            if (edge) {
                rows[static_cast<std::size_t>(z)][static_cast<std::size_t>(x)] = '#';
            }
        }
    }
    return rows;
}

void stampLayout(MapRows& rows, const std::vector<std::string_view>& layout, int offsetX,
                 int offsetZ) {
    if (layout.empty()) {
        return;
    }

    const std::size_t layoutWidth = layout.front().size();
    for (const std::string_view line : layout) {
        if (line.size() != layoutWidth) {
            throw std::invalid_argument("Layout rows have different lengths.");
        }
    }

    const int depth = static_cast<int>(rows.size());
    const int width = depth > 0 ? static_cast<int>(rows.front().size()) : 0;
    const bool fits = offsetX >= 0 && offsetZ >= 0 &&
                      offsetX + static_cast<int>(layoutWidth) <= width &&
                      offsetZ + static_cast<int>(layout.size()) <= depth;
    if (!fits) {
        throw std::invalid_argument("Layout does not fit inside the world at that offset.");
    }

    for (std::size_t z = 0; z < layout.size(); ++z) {
        for (std::size_t x = 0; x < layoutWidth; ++x) {
            const char c = layout[z][x];
            if (c != ' ') {
                rows[z + static_cast<std::size_t>(offsetZ)][x + static_cast<std::size_t>(offsetX)] = c;
            }
        }
    }
}

World buildStartingWorld(std::uint32_t seed) {
    MapRows rows = generateTerrain(seed, data::kWorldWidth, data::kWorldDepth);
    stampLayout(rows,
                std::vector<std::string_view>(data::kTownLayout.begin(), data::kTownLayout.end()),
                data::kTownOffsetX, data::kTownOffsetZ);
    return World::fromRows(rows);
}

} // namespace game
