// World.h
//
// The static layout of the game world: a rectangular grid of tiles.
//
// World answers questions like "what tile is at (x, y)?" and "can something
// stand here?". It owns no graphics; the renderer reads it and decides how
// each tile type looks.

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "Vec2.h"

namespace game {

// Every kind of ground the world can contain.
enum class Tile : std::uint8_t {
    Grass,
    Path,
    Water,
    Wall,
};

// Rule: which tiles block movement. Lives with the tile definitions so the
// renderer never has to decide what is solid.
bool isSolid(Tile tile);

class World {
public:
    // Builds the world from rows of map characters (see GameData.h for the
    // legend). Throws std::runtime_error if the map is empty, rows have
    // different lengths, a character is unknown, or there is not exactly
    // one '@' start marker.
    template <std::size_t N>
    static World fromRows(const std::array<std::string_view, N>& rows) {
        return fromRows(std::vector<std::string_view>(rows.begin(), rows.end()));
    }
    static World fromRows(const std::vector<std::string_view>& rows);

    int width() const { return width_; }
    int height() const { return height_; }

    // Tile at grid cell (col, row). Cells outside the map count as Wall,
    // so nothing can ever walk off the edge of the world.
    Tile tileAt(int col, int row) const;

    // Centre of the tile marked '@' in the map, in world units.
    Vec2 playerStart() const { return playerStart_; }

private:
    World(int width, int height, std::vector<Tile> tiles, Vec2 playerStart);

    int width_;
    int height_;
    std::vector<Tile> tiles_; // row-major: index = row * width_ + col
    Vec2 playerStart_;
};

} // namespace game
