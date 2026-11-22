// BuildingData.h
//
// Reads data/buildings.txt: which kit pieces each house style is built
// from, which roofs exist, and which style and door side each building in
// the town gets. It is presentation data (model files), so it lives in
// platform/, but it is plain C++ with NO raylib, so the tests can check it
// without any assets.
//
// Example (see data/buildings.txt for the real file):
//
//     # comment lines start with '#'
//     [style]
//     name         = plaster
//     wall         = Wall_Plaster_Straight.gltf
//     window       = Wall_Plaster_Window_Wide_Round.gltf
//     window_frame = Window_Wide_Round1.gltf
//     door         = Wall_Plaster_Door_Round.gltf
//     door_leaf    = Door_1_Round.gltf
//     corner       = Corner_Exterior_Wood.gltf
//     scale        = 1.0
//
//     [roof]
//     name  = tiles 6x8
//     model = Roof_RoundTiles_6x8.gltf
//     gable = Roof_Front_Brick6.gltf
//     fits  = 6, 8
//
//     [building]
//     tile   = 29, 24
//     style  = plaster
//     facing = 180
//     offset = 0, 0, 0
//     roof   = tiles 6x8
//
// Style keys: wall/window/door are 2 m wall panels (plain, with a window
// opening, with a doorway); window_frame fills the window opening and
// door_leaf the doorway; corner is the post on each outside corner. scale
// is an optional height scale for the whole house.
//
// Roof keys: model is the roof, gable the wall piece that closes the
// triangle under each end of the ridge. fits is the footprint the roof was
// made for: across the ridge, then along it (tiles). The ridge must run
// along the model's own Z axis, as it does for the kit's round-tile roofs.
//
// Building keys: tile is any WORLD tile of the building. facing is the side
// the door is on: 0 south (+Z), 90 east (+X), 180 north, 270 west (the same
// angles as NPC facing). offset is an optional nudge in tiles (x, y, z).
// roof is optional: without it the roof whose `fits` is closest to the
// building's footprint is used.
//
// Model files are looked up in assets/medieval/. Every key is required
// except scale, offset and a building's roof. Unknown keys, repeated keys,
// bad values and names of styles or roofs that do not exist are errors,
// reported as "file:line: problem", so typos never fail silently.
// Comments must be on their own line, as in data/npcs.txt.

#pragma once

#include <istream>
#include <string>
#include <vector>

namespace platform {

// One house style: the kit pieces its walls are assembled from.
struct BuildingStyle {
    std::string name;
    std::string wall;        // plain wall panel
    std::string window;      // wall panel with a window opening
    std::string windowFrame; // the window that fills that opening
    std::string door;        // wall panel with a doorway
    std::string doorLeaf;    // the door that fills the doorway
    std::string corner;      // post standing on each outside corner
    float heightScale = 1.0f;
    int line = 0;            // line of its [style], for error messages
};

// One roof model and the gable piece that goes with it.
struct BuildingRoof {
    std::string name;
    std::string model;
    std::string gable;
    int fitsAcross = 0; // footprint the model was made for, in tiles:
    int fitsAlong = 0;  // across the ridge (model X), along it (model Z)
    int line = 0;
};

// One building in the town map, picked out by any tile it covers.
struct BuildingEntry {
    int tileX = 0;
    int tileZ = 0;
    std::string style;
    std::string roof;      // empty = pick the best-fitting roof
    int facingDegrees = 0; // 0, 90, 180 or 270: the side the door is on
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    int line = 0;          // line of its [building], for error messages
};

struct BuildingData {
    std::string source; // file name, for error messages found later
    std::vector<BuildingStyle> styles;
    std::vector<BuildingRoof> roofs;
    std::vector<BuildingEntry> buildings;

    // Lookup by name. nullptr if there is no such style or roof.
    const BuildingStyle* findStyle(const std::string& name) const;
    const BuildingRoof* findRoof(const std::string& name) const;
};

// Parses `in`. `sourceName` is used in error messages.
// Throws std::runtime_error ("source:line: problem") on bad input.
BuildingData parseBuildings(std::istream& in, const std::string& sourceName);

// Opens `path` and parses it. Throws std::runtime_error if the file cannot
// be opened or is invalid.
BuildingData loadBuildingFile(const std::string& path);

} // namespace platform
