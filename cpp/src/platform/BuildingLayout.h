// BuildingLayout.h
//
// Works out WHERE each kit piece of an assembled building goes: which wall
// panels run along which side, where the door is, where the corner posts
// and the roof sit. Pure maths on the tile footprint, with NO raylib, so it
// is tested without any assets. BuildingModels.cpp turns the result into
// meshes.
//
// It never changes the game: the footprint (and so collision) comes from
// the town map, and the pieces are fitted to it.

#pragma once

#include <cstddef>
#include <vector>

#include "../game/World.h"
#include "BuildingData.h"

namespace platform {

// Facts about the Medieval Village MegaKit pieces, measured from the files.
namespace kit {
// Wall panels are 2 m wide, centred on x = 0, standing on y = 0. Their
// outside face is the plane z = 0 and they face +Z; the wall's thickness
// is behind that face (inside the house).
constexpr float kPanelWidth = 2.0f;
// Top of the plaster on a wall panel; roofs are placed with their origin
// here (their eaves hang below it, outside the walls).
constexpr float kWallTop = 3.0f;
// Where the door leaf goes inside a door panel (its hinge, in the panel's
// own coordinates): left edge of the doorway, in the middle of the wall.
constexpr float kDoorLeafX = -0.53f;
constexpr float kDoorLeafZ = -0.1f;
} // namespace kit

// A building's rectangle of tiles: x0..x0+width-1, z0..z0+depth-1.
struct Footprint {
    int x0 = 0;
    int z0 = 0;
    int width = 0;
    int depth = 0;
    bool rectangular = true; // false if its tiles do not fill the rectangle
};

// Every building in the world. Touching Building tiles form one building.
struct BuildingMap {
    int worldWidth = 0;
    std::vector<int> ids;               // per tile (z * worldWidth + x): building index or -1
    std::vector<Footprint> footprints;  // by building index

    // Building index at (x, z), or -1 if none (or outside the world).
    int idAt(int x, int z) const;
};

BuildingMap findBuildings(const game::World& world);

// A building from the data file matched to a building in the world.
struct PlacedBuilding {
    std::size_t building = 0; // index into BuildingMap::footprints
    BuildingEntry entry;
    BuildingStyle style;
};

// Matches each [building] to the building covering its tile. Throws
// std::runtime_error ("file:line: problem") if the tile is not a building,
// two entries pick the same building, or the building is not a plain
// rectangle (only rectangles can be assembled from wall panels yet).
std::vector<PlacedBuilding> matchBuildings(const BuildingMap& map, const BuildingData& data);

// Which kit piece a placement uses.
enum class Piece { Wall, Window, Door, DoorLeaf, Corner, Roof };

// One piece in the world. To place it: scale it (in its own axes), turn it
// by yawDegrees around Y (0 = its +Z faces south/+Z, 90 = faces east/+X,
// the same angles as everywhere else), then move it to (x, y, z).
struct PiecePlacement {
    Piece piece = Piece::Wall;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yawDegrees = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;
};

// The world tile just outside the door side where the door should go: the
// path or plaza tile touching that side closest to its middle. Returns the
// position ALONG the side (x for north/south, z for east/west), or -1 if no
// path touches that side.
int findDoorTile(const game::World& world, const Footprint& footprint, int facingDegrees);

// Lays out the pieces of one building:
//   - each side is split into panels of about 2 m (stretched to fit);
//   - the door side gets a door panel (+ door leaf) at `doorTile` (see
//     findDoorTile; -1 = middle of the side);
//   - every other panel alternates window / plain wall;
//   - a corner post on each corner and the roof stretched over the top.
std::vector<PiecePlacement> planBuilding(const Footprint& footprint, const BuildingStyle& style,
                                         const BuildingEntry& entry, int doorTile);

} // namespace platform
