// BuildingModels.cpp
//
// Steps, all at startup:
//   1. Load every piece model each used style and roof needs (each file
//      once, see MergedModels).
//   2. For every placed building, plan its pieces (BuildingLayout) and add
//      each piece, moved/turned/stretched into place, to the merged meshes.
//   3. Upload the merged meshes. draw() then sends one mesh per material.

#include "BuildingModels.h"

#include <iostream>
#include <map>

// raymath.h is C code; its `= { 0 }` initialisers trip -Wextra in C++.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop

namespace platform {

namespace {

// The model-space -> world-space matrix for a placement: stretch in the
// piece's own axes, turn around Y, then move into place.
Matrix placementMatrix(const PiecePlacement& p) {
    const Matrix scale = MatrixScale(p.scaleX, p.scaleY, p.scaleZ);
    const Matrix turn = MatrixRotateY(p.yawDegrees * DEG2RAD);
    const Matrix move = MatrixTranslate(p.x, p.y, p.z);
    return MatrixMultiply(MatrixMultiply(scale, turn), move);
}

} // namespace

BuildingModels::BuildingModels(const game::World& world, const BuildingMap& map,
                               const std::vector<PlacedBuilding>& placed, const std::string& assetDir)
    : models_(assetDir, "Buildings", false), modelled_(map.footprints.size(), false) {
    for (const PlacedBuilding& building : placed) {
        const BuildingStyle& style = building.style;
        const std::map<Piece, std::string> files = {
            {Piece::Wall, style.wall},               {Piece::Window, style.window},
            {Piece::WindowFrame, style.windowFrame}, {Piece::Door, style.door},
            {Piece::DoorLeaf, style.doorLeaf},       {Piece::Corner, style.corner},
            {Piece::Roof, building.roof.model},      {Piece::Gable, building.roof.gable},
        };
        bool allLoaded = true;
        for (const auto& [piece, file] : files) {
            allLoaded = models_.load(file) && allLoaded; // load every file, report each missing one
        }
        if (!allLoaded) {
            std::cerr << "Buildings: drawing the building at tile " << building.entry.tileX << ", "
                      << building.entry.tileZ << " as a box instead.\n";
            continue;
        }
        modelled_[building.building] = true;

        const Footprint& footprint = map.footprints[building.building];
        const int doorTile = findDoorTile(world, footprint, building.entry.facingDegrees);
        for (const PiecePlacement& placement : planBuilding(footprint, style, building.roof, building.entry, doorTile)) {
            models_.add(files.at(placement.piece), placementMatrix(placement));
        }
    }
    models_.finish();
}

} // namespace platform
