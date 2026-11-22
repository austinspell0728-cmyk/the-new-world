// World.h
//
// The static layout of the game world: a rectangular grid of tiles lying on
// the ground plane (X/Z, see Vec3.h).
//
// World answers questions like "what tile is at this cell?" and "can
// something stand here?". It owns no graphics; the renderer reads it and
// decides how each tile type looks. To see how the starting world is put
// together (terrain + hand-made town), see WorldBuilder.h.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Vec3.h"

namespace game {

// Every kind of tile the world can contain. The map character for each is
// listed in GameData.h.
enum class Tile : std::uint8_t {
    Grass,
    Path,
    Plaza,    // market square paving
    Water,
    Wall,
    Building, // touching building tiles form one building
    Tree,
    Stall,    // market stall
    Well,
};

// Rule: which tiles block movement. Lives with the tile definitions so the
// renderer never has to decide what is solid. Every solid tile blocks its
// whole square, even if it is drawn smaller (a tree trunk, a stall).
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
    static World fromRows(const std::vector<std::string>& rows) {
        return fromRows(std::vector<std::string_view>(rows.begin(), rows.end()));
    }
    static World fromRows(const std::vector<std::string_view>& rows);

    // Size of the map: width runs along X (columns), depth along Z (rows).
    int width() const { return width_; }
    int depth() const { return depth_; }

    // Tile at grid cell (x, z), i.e. column x of row z in the text map.
    // Cells outside the map count as Wall, so nothing can ever walk off the
    // edge of the world.
    Tile tileAt(int x, int z) const;

    // True if the tile under ground point (x, z) can be walked on.
    bool isWalkableAt(float x, float z) const;

    // Centre of the tile marked '@', at ground height (y = 0), in tiles.
    Vec3 playerStart() const { return playerStart_; }

private:
    World(int width, int depth, std::vector<Tile> tiles, Vec3 playerStart);

    int width_;
    int depth_;
    std::vector<Tile> tiles_; // row-major: index = z * width_ + x
    Vec3 playerStart_;
};

} // namespace game
