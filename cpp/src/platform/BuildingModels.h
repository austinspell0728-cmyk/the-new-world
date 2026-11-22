// BuildingModels.h
//
// Draws the buildings listed in data/buildings.txt with Medieval Village
// MegaKit pieces instead of flat boxes.
//
// At startup each piece model is loaded ONCE. Every piece of every building
// is then copied, already moved into place, into a few big meshes: one per
// material (plaster, wood trim, roof tiles, ...), shared by all buildings
// (see MergedModels). So the frame draws a handful of meshes, not hundreds
// of separate pieces.
//
// If a style's piece file is missing or fails to load, the buildings using
// that style are left out here, and WorldMesh keeps drawing them as boxes.

#pragma once

#include <string>
#include <vector>

#include "BuildingLayout.h"
#include "MergedModels.h"

namespace platform {

class BuildingModels {
public:
    // Loads the pieces from `assetDir` (ending in '/') and builds the meshes
    // for `placed`. The window must already be open.
    BuildingModels(const game::World& world, const BuildingMap& map,
                   const std::vector<PlacedBuilding>& placed, const std::string& assetDir);

    // Per building (index into BuildingMap::footprints): true if it is drawn
    // here, false if it should still be drawn as a box.
    const std::vector<bool>& modelled() const { return modelled_; }

    // Number of meshes draw() sends each frame (for the benchmark).
    int drawCount() const { return models_.drawCount(); }

    // Call between BeginMode3D and EndMode3D.
    void draw() const { models_.draw(); }

private:
    MergedModels models_;
    std::vector<bool> modelled_;
};

} // namespace platform
