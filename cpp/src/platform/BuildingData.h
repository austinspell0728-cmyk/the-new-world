// BuildingData.h
//
// Reads data/buildings.txt: which kit pieces each house style is built
// from, and which style, door side and nudge each building in the town gets.
// It is presentation data (model files), so it lives in platform/, but it is
// plain C++ with NO raylib, so the tests can check it without any assets.
//
// Example (see data/buildings.txt for the real file):
//
//     # comment lines start with '#'
//     [style]
//     name      = plaster
//     wall      = Wall_Plaster_Straight.gltf
//     window    = Wall_Plaster_Window_Wide_Round.gltf
//     door      = Wall_Plaster_Door_Round.gltf
//     door_leaf = Door_1_Round.gltf
//     corner    = Corner_Exterior_Wood.gltf
//     roof      = Roof_RoundTiles_6x8.gltf
//     roof_fits = 6, 8
//     scale     = 1.0
//
//     [building]
//     tile   = 29, 24
//     style  = plaster
//     facing = 180
//     offset = 0, 0, 0
//
// Style keys: wall/window/door are 2 m wall panels (plain, with a window,
// with a doorway), door_leaf fills the doorway, corner is the post on each
// outside corner. roof_fits is the footprint (x, z in tiles) the roof model
// was made for; it is stretched to the real footprint. scale is an optional
// height scale for the whole house. Model files are looked up in
// assets/medieval/.
//
// Building keys: tile is any WORLD tile of the building. facing is the side
// the door is on: 0 south (+Z), 90 east (+X), 180 north, 270 west (the same
// angles as NPC facing). offset is an optional nudge in tiles (x, y, z).
//
// Every key is required except `scale` and `offset`. Unknown keys,
// repeated keys, bad values and a building naming an unknown style are
// errors, reported as "file:line: problem", so typos never fail silently.
// Comments must be on their own line, as in data/npcs.txt.

#pragma once

#include <istream>
#include <string>
#include <vector>

namespace platform {

// One house style: the kit pieces it is assembled from.
struct BuildingStyle {
    std::string name;
    std::string wall;     // plain wall panel
    std::string window;   // wall panel with a window
    std::string door;     // wall panel with a doorway
    std::string doorLeaf; // the door that fills the doorway
    std::string corner;   // post standing on each outside corner
    std::string roof;
    int roofFitsX = 0;    // footprint the roof model was made for, in tiles;
    int roofFitsZ = 0;    // the roof is stretched from this to the real one
    float heightScale = 1.0f;
    int line = 0;         // line of its [style], for error messages
};

// One building in the town map, picked out by any tile it covers.
struct BuildingEntry {
    int tileX = 0;
    int tileZ = 0;
    std::string style;
    int facingDegrees = 0; // 0, 90, 180 or 270: the side the door is on
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    int line = 0;          // line of its [building], for error messages
};

struct BuildingData {
    std::string source; // file name, for error messages found later
    std::vector<BuildingStyle> styles;
    std::vector<BuildingEntry> buildings;

    // Lookup by style name. nullptr if there is no such style.
    const BuildingStyle* findStyle(const std::string& name) const;
};

// Parses `in`. `sourceName` is used in error messages.
// Throws std::runtime_error ("source:line: problem") on bad input.
BuildingData parseBuildings(std::istream& in, const std::string& sourceName);

// Opens `path` and parses it. Throws std::runtime_error if the file cannot
// be opened or is invalid.
BuildingData loadBuildingFile(const std::string& path);

} // namespace platform
