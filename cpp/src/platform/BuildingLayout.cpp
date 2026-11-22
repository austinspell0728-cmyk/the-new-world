// BuildingLayout.cpp

#include "BuildingLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <queue>
#include <stdexcept>
#include <string>

namespace platform {

int BuildingMap::idAt(int x, int z) const {
    if (worldWidth <= 0 || x < 0 || z < 0 || x >= worldWidth) {
        return -1;
    }
    const auto index = static_cast<std::size_t>(z) * static_cast<std::size_t>(worldWidth) +
                       static_cast<std::size_t>(x);
    return index < ids.size() ? ids[index] : -1;
}

BuildingMap findBuildings(const game::World& world) {
    BuildingMap map;
    const int width = world.width(), depth = world.depth();
    map.worldWidth = width;
    map.ids.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(depth), -1);
    auto index = [width](int x, int z) { return static_cast<std::size_t>(z * width + x); };

    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            if (world.tileAt(x, z) != game::Tile::Building || map.ids[index(x, z)] != -1) {
                continue;
            }
            // Flood fill from this tile across its touching building tiles,
            // tracking the bounding rectangle as we go.
            const int id = static_cast<int>(map.footprints.size());
            int minX = x, maxX = x, minZ = z, maxZ = z, count = 0;
            std::queue<std::array<int, 2>> open;
            open.push({x, z});
            map.ids[index(x, z)] = id;
            while (!open.empty()) {
                const auto [cx, cz] = open.front();
                open.pop();
                ++count;
                minX = std::min(minX, cx); maxX = std::max(maxX, cx);
                minZ = std::min(minZ, cz); maxZ = std::max(maxZ, cz);
                for (const auto& [dx, dz] : {std::array<int, 2>{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
                    const int nx = cx + dx, nz = cz + dz;
                    if (nx >= 0 && nz >= 0 && nx < width && nz < depth &&
                        world.tileAt(nx, nz) == game::Tile::Building && map.ids[index(nx, nz)] == -1) {
                        map.ids[index(nx, nz)] = id;
                        open.push({nx, nz});
                    }
                }
            }
            Footprint f;
            f.x0 = minX;
            f.z0 = minZ;
            f.width = maxX - minX + 1;
            f.depth = maxZ - minZ + 1;
            f.rectangular = (count == f.width * f.depth);
            map.footprints.push_back(f);
        }
    }
    return map;
}

std::vector<PlacedBuilding> matchBuildings(const BuildingMap& map, const BuildingData& data) {
    std::vector<PlacedBuilding> placed;
    std::vector<int> claimedBy(map.footprints.size(), 0); // line of the entry using it, 0 = free
    for (const BuildingEntry& entry : data.buildings) {
        auto fail = [&](const std::string& message) {
            throw std::runtime_error(data.source + ":" + std::to_string(entry.line) + ": " + message);
        };
        const std::string tile = std::to_string(entry.tileX) + ", " + std::to_string(entry.tileZ);
        const int id = map.idAt(entry.tileX, entry.tileZ);
        if (id < 0) {
            fail("tile " + tile + " is not part of a building");
        }
        const auto index = static_cast<std::size_t>(id);
        if (claimedBy[index] != 0) {
            fail("tile " + tile + " is the same building as the [building] on line " +
                 std::to_string(claimedBy[index]));
        }
        if (!map.footprints[index].rectangular) {
            fail("the building at tile " + tile +
                 " is not a plain rectangle, and only rectangles can be assembled from wall panels");
        }
        claimedBy[index] = entry.line;
        const BuildingStyle* style = data.findStyle(entry.style);
        if (style == nullptr) { // parseBuildings already checks this; kept for safety
            fail("unknown style '" + entry.style + "'");
        }
        placed.push_back({index, entry, *style});
    }
    return placed;
}

namespace {

// One side of the footprint: where it runs and which way it faces.
struct Side {
    int facing;       // 0 south, 90 east, 180 north, 270 west
    float startX, startZ; // one end of the side, in world tiles
    float runX, runZ;     // unit direction along the side
    float length;         // in tiles
};

Side sideFacing(const Footprint& f, int facing) {
    const float x0 = static_cast<float>(f.x0), z0 = static_cast<float>(f.z0);
    const float x1 = x0 + static_cast<float>(f.width), z1 = z0 + static_cast<float>(f.depth);
    switch (facing) {
    case 0: return {0, x0, z1, 1, 0, static_cast<float>(f.width)};    // south edge, z = z1
    case 90: return {90, x1, z0, 0, 1, static_cast<float>(f.depth)};  // east edge, x = x1
    case 180: return {180, x0, z0, 1, 0, static_cast<float>(f.width)}; // north edge, z = z0
    default: return {270, x0, z0, 0, 1, static_cast<float>(f.depth)}; // west edge, x = x0
    }
}

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

} // namespace

int findDoorTile(const game::World& world, const Footprint& f, int facingDegrees) {
    const bool alongX = (facingDegrees == 0 || facingDegrees == 180);
    const int first = alongX ? f.x0 : f.z0;
    const int count = alongX ? f.width : f.depth;
    // The row (or column) of tiles just outside that side.
    const int outside = facingDegrees == 0     ? f.z0 + f.depth
                        : facingDegrees == 90  ? f.x0 + f.width
                        : facingDegrees == 180 ? f.z0 - 1
                                               : f.x0 - 1;
    const float middle = static_cast<float>(first) + static_cast<float>(count - 1) / 2.0f;

    int best = -1;
    for (int i = first; i < first + count; ++i) {
        const game::Tile tile = alongX ? world.tileAt(i, outside) : world.tileAt(outside, i);
        if (tile != game::Tile::Path && tile != game::Tile::Plaza) {
            continue;
        }
        if (best < 0 || std::abs(static_cast<float>(i) - middle) < std::abs(static_cast<float>(best) - middle)) {
            best = i;
        }
    }
    return best;
}

std::vector<PiecePlacement> planBuilding(const Footprint& f, const BuildingStyle& style,
                                         const BuildingEntry& entry, int doorTile) {
    std::vector<PiecePlacement> pieces;
    const float height = style.heightScale;
    auto add = [&](Piece piece, float x, float y, float z, float yaw, float sx, float sy, float sz) {
        pieces.push_back({piece, x + entry.offsetX, y + entry.offsetY, z + entry.offsetZ, yaw, sx, sy, sz});
    };

    // Walls: each side split into panels of about kPanelWidth, stretched so
    // a whole number of them exactly fills the side.
    for (const int facing : {0, 90, 180, 270}) {
        const Side side = sideFacing(f, facing);
        const int panels = std::max(1, static_cast<int>(std::lround(side.length / kit::kPanelWidth)));
        const float panelLength = side.length / static_cast<float>(panels);
        const float stretch = panelLength / kit::kPanelWidth;

        // Which panel holds the door: the one over the door tile's centre.
        int doorPanel = -1;
        if (facing == entry.facingDegrees) {
            const float sideStart = (side.runX != 0.0f) ? side.startX : side.startZ;
            const float target = (doorTile >= 0) ? static_cast<float>(doorTile) + 0.5f
                                                 : sideStart + side.length / 2.0f;
            doorPanel = std::clamp(static_cast<int>((target - sideStart) / panelLength), 0, panels - 1);
        }

        const float yaw = static_cast<float>(facing);
        for (int i = 0; i < panels; ++i) {
            const float along = (static_cast<float>(i) + 0.5f) * panelLength;
            const float x = side.startX + side.runX * along;
            const float z = side.startZ + side.runZ * along;
            if (i == doorPanel) {
                add(Piece::Door, x, 0.0f, z, yaw, stretch, height, 1.0f);
                // The door leaf sits inside the doorway: offset in the
                // panel's own axes, turned the same way as the panel.
                const float yawRad = yaw * kDegToRad;
                const float lx = kit::kDoorLeafX * stretch, lz = kit::kDoorLeafZ;
                add(Piece::DoorLeaf, x + lx * std::cos(yawRad) + lz * std::sin(yawRad), 0.0f,
                    z - lx * std::sin(yawRad) + lz * std::cos(yawRad), yaw, stretch, height, 1.0f);
            } else {
                add(i % 2 == 0 ? Piece::Window : Piece::Wall, x, 0.0f, z, yaw, stretch, height, 1.0f);
            }
        }
    }

    // Corner posts.
    const float x0 = static_cast<float>(f.x0), z0 = static_cast<float>(f.z0);
    const float x1 = x0 + static_cast<float>(f.width), z1 = z0 + static_cast<float>(f.depth);
    for (const auto& [cx, cz] : {std::array<float, 2>{x0, z0}, {x1, z0}, {x1, z1}, {x0, z1}}) {
        add(Piece::Corner, cx, 0.0f, cz, 0.0f, 1.0f, height, 1.0f);
    }

    // Roof: stretched from the footprint it was made for to this one. If
    // the building is long the other way from the roof, turn the roof 90
    // degrees; then its own X runs along the world's Z.
    const bool buildingLongInX = f.width > f.depth;
    const bool roofLongInX = style.roofFitsX > style.roofFitsZ;
    const bool turn = (f.width != f.depth) && (style.roofFitsX != style.roofFitsZ) &&
                      (buildingLongInX != roofLongInX);
    const float alongModelX = static_cast<float>(turn ? f.depth : f.width);
    const float alongModelZ = static_cast<float>(turn ? f.width : f.depth);
    add(Piece::Roof, (x0 + x1) / 2.0f, kit::kWallTop * height, (z0 + z1) / 2.0f, turn ? 90.0f : 0.0f,
        alongModelX / static_cast<float>(style.roofFitsX), height,
        alongModelZ / static_cast<float>(style.roofFitsZ));

    return pieces;
}

} // namespace platform
