// TreeModels.h
//
// Draws the town's trees with Stylized Nature MegaKit models instead of the
// old box trees. Which model, turn and size each tree tile gets comes from
// TreeLayout (a hash of the tile), the models from data/trees.txt.
//
// Every tree is baked into a few merged meshes at startup, one per
// material (bark, leaves, pine needles), see MergedModels. The leaves are
// flat cards with see-through parts, so they are drawn with the alpha
// cut-out shader.
//
// If a variant's model is missing or fails to load, the tiles that picked
// it are left out here and WorldMesh keeps drawing the old box tree there.

#pragma once

#include <string>
#include <vector>

#include "MergedModels.h"
#include "TreeLayout.h"

namespace platform {

class TreeModels {
public:
    // Loads the variants' models from `assetDir` (ending in '/') and builds
    // the meshes for `trees`. The window must already be open.
    TreeModels(const game::World& world, const TreeData& data, const std::vector<TreePlacement>& trees,
               const std::string& assetDir);

    // Per tile (z * width + x): true if a model tree is drawn there, false
    // if WorldMesh should draw the box tree (or there is no tree).
    const std::vector<bool>& modelledTiles() const { return modelledTiles_; }

    // Number of meshes draw() sends each frame (for the benchmark).
    int drawCount() const { return models_.drawCount(); }

    // Call between BeginMode3D and EndMode3D.
    void draw() const { models_.draw(); }

private:
    MergedModels models_;
    std::vector<bool> modelledTiles_;
};

} // namespace platform
