// BuildingModels.h
//
// Draws the buildings listed in data/buildings.txt with Medieval Village
// MegaKit pieces instead of flat boxes.
//
// At startup each piece model is loaded ONCE. Every piece of every building
// is then copied, already moved into place, into a few big meshes: one per
// material (plaster, wood trim, roof tiles, ...), shared by all buildings.
// So the frame draws a handful of meshes, not hundreds of separate pieces.
// The town never changes at runtime, so this is done once.
//
// If a style's piece file is missing or fails to load, the buildings using
// that style are left out here, and WorldMesh keeps drawing them as boxes.

#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "BuildingLayout.h"

namespace platform {

class BuildingModels {
public:
    // Loads the pieces from `assetDir` (ending in '/') and builds the meshes
    // for `placed`. The window must already be open.
    BuildingModels(const game::World& world, const BuildingMap& map,
                   const std::vector<PlacedBuilding>& placed, const std::string& assetDir);
    ~BuildingModels();

    // Owns GPU memory, so it cannot be copied.
    BuildingModels(const BuildingModels&) = delete;
    BuildingModels& operator=(const BuildingModels&) = delete;

    // Per building (index into BuildingMap::footprints): true if it is drawn
    // here, false if it should still be drawn as a box.
    const std::vector<bool>& modelled() const { return modelled_; }

    // Number of meshes draw() sends each frame (for the report/benchmark).
    int drawCount() const { return static_cast<int>(batches_.size()); }

    // Call between BeginMode3D and EndMode3D.
    void draw() const;

private:
    // One merged mesh, drawn with the material (texture) of the pieces it
    // was made from. The material belongs to a piece model in sources_.
    struct Batch {
        Mesh mesh;
        Material material;
    };

    std::vector<Model> sources_; // the loaded piece models, kept for their textures
    std::vector<Batch> batches_;
    std::vector<bool> modelled_;
};

} // namespace platform
