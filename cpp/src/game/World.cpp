// World.cpp
//
// Loading the tile grid from text and answering tile queries.

#include "World.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace game {

bool isSolid(Tile tile) {
    switch (tile) {
    case Tile::Wall:
    case Tile::Water:
        return true;
    case Tile::Grass:
    case Tile::Path:
        return false;
    }
    return true; // Unreachable for valid enums; treat unknown as solid to be safe.
}

World::World(int width, int height, std::vector<Tile> tiles, Vec2 playerStart)
    : width_(width), height_(height), tiles_(std::move(tiles)), playerStart_(playerStart) {}

World World::fromRows(const std::vector<std::string_view>& rows) {
    if (rows.empty() || rows.front().empty()) {
        throw std::runtime_error("World map is empty.");
    }

    const int width = static_cast<int>(rows.front().size());
    const int height = static_cast<int>(rows.size());

    std::vector<Tile> tiles;
    tiles.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    Vec2 playerStart{};
    int startMarkers = 0;

    for (int row = 0; row < height; ++row) {
        const std::string_view line = rows[static_cast<std::size_t>(row)];

        if (static_cast<int>(line.size()) != width) {
            throw std::runtime_error("World map row " + std::to_string(row) +
                                     " has a different length from row 0.");
        }

        for (int col = 0; col < width; ++col) {
            switch (line[static_cast<std::size_t>(col)]) {
            case '#': tiles.push_back(Tile::Wall); break;
            case '~': tiles.push_back(Tile::Water); break;
            case '.': tiles.push_back(Tile::Grass); break;
            case ',': tiles.push_back(Tile::Path); break;
            case '@':
                // The start marker is ordinary walkable ground. The player
                // spawns in the centre of the tile, hence the + 0.5.
                tiles.push_back(Tile::Path);
                playerStart = {static_cast<float>(col) + 0.5f, static_cast<float>(row) + 0.5f};
                ++startMarkers;
                break;
            default:
                throw std::runtime_error("Unknown map character at row " + std::to_string(row) +
                                         ", column " + std::to_string(col) + ".");
            }
        }
    }

    if (startMarkers != 1) {
        throw std::runtime_error("World map must contain exactly one '@' start marker.");
    }

    return World(width, height, std::move(tiles), playerStart);
}

Tile World::tileAt(int col, int row) const {
    if (col < 0 || row < 0 || col >= width_ || row >= height_) {
        return Tile::Wall;
    }
    return tiles_[static_cast<std::size_t>(row) * static_cast<std::size_t>(width_) +
                  static_cast<std::size_t>(col)];
}

} // namespace game
