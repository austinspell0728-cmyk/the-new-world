// World.cpp
//
// Loading the tile grid from text and answering tile queries.
// Text map column = X, text map row = Z (see Vec3.h).

#include "World.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace game {

bool isSolid(Tile tile) {
    switch (tile) {
    case Tile::Wall:
    case Tile::Water:
    case Tile::Building:
    case Tile::Tree:
    case Tile::Stall:
    case Tile::Well:
        return true;
    case Tile::Grass:
    case Tile::Path:
    case Tile::Plaza:
        return false;
    }
    return true; // Unreachable for valid enums; treat unknown as solid to be safe.
}

World::World(int width, int depth, std::vector<Tile> tiles, Vec3 playerStart)
    : width_(width), depth_(depth), tiles_(std::move(tiles)), playerStart_(playerStart) {}

World World::fromRows(const std::vector<std::string_view>& rows) {
    if (rows.empty() || rows.front().empty()) {
        throw std::runtime_error("World map is empty.");
    }

    const int width = static_cast<int>(rows.front().size());
    const int depth = static_cast<int>(rows.size());

    std::vector<Tile> tiles;
    tiles.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(depth));

    Vec3 playerStart{};
    int startMarkers = 0;

    for (int z = 0; z < depth; ++z) {
        const std::string_view line = rows[static_cast<std::size_t>(z)];

        if (static_cast<int>(line.size()) != width) {
            throw std::runtime_error("World map row " + std::to_string(z) +
                                     " has a different length from row 0.");
        }

        for (int x = 0; x < width; ++x) {
            switch (line[static_cast<std::size_t>(x)]) {
            case '#': tiles.push_back(Tile::Wall); break;
            case '~': tiles.push_back(Tile::Water); break;
            case '.': tiles.push_back(Tile::Grass); break;
            case ',': tiles.push_back(Tile::Path); break;
            case ':': tiles.push_back(Tile::Plaza); break;
            case 'B': tiles.push_back(Tile::Building); break;
            case 'T': tiles.push_back(Tile::Tree); break;
            case 'S': tiles.push_back(Tile::Stall); break;
            case 'W': tiles.push_back(Tile::Well); break;
            case '@':
                // The start marker is ordinary walkable ground. The player
                // spawns in the centre of the tile (hence + 0.5), standing
                // on the ground (y = 0).
                tiles.push_back(Tile::Path);
                playerStart = {static_cast<float>(x) + 0.5f, 0.0f, static_cast<float>(z) + 0.5f};
                ++startMarkers;
                break;
            default:
                throw std::runtime_error("Unknown map character at row " + std::to_string(z) +
                                         ", column " + std::to_string(x) + ".");
            }
        }
    }

    if (startMarkers != 1) {
        throw std::runtime_error("World map must contain exactly one '@' start marker.");
    }

    return World(width, depth, std::move(tiles), playerStart);
}

Tile World::tileAt(int x, int z) const {
    if (x < 0 || z < 0 || x >= width_ || z >= depth_) {
        return Tile::Wall;
    }
    return tiles_[static_cast<std::size_t>(z) * static_cast<std::size_t>(width_) +
                  static_cast<std::size_t>(x)];
}

} // namespace game
