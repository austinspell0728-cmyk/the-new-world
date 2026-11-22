// TreeModels.cpp

#include "TreeModels.h"

#include <iostream>

// raymath.h is C code; its `= { 0 }` initialisers trip -Wextra in C++.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop

namespace platform {

TreeModels::TreeModels(const game::World& world, const TreeData& data, const std::vector<TreePlacement>& trees,
                       const std::string& assetDir)
    : models_(assetDir, "Trees", true),
      modelledTiles_(static_cast<std::size_t>(world.width()) * static_cast<std::size_t>(world.depth()), false) {
    // Load each variant's model once; remember which ones work.
    std::vector<bool> variantLoaded;
    for (const TreeVariant& variant : data.variants) {
        variantLoaded.push_back(models_.load(variant.model));
    }

    std::vector<int> boxTrees(data.variants.size(), 0); // per variant that failed
    for (const TreePlacement& tree : trees) {
        const TreeVariant& variant = data.variants[tree.variant];
        if (!variantLoaded[tree.variant]) {
            ++boxTrees[tree.variant];
            continue;
        }
        // Scale, turn, then move the model's origin into place (TreeLayout
        // already shifted it so the trunk lands on the tile centre).
        const Matrix toWorld = MatrixMultiply(
            MatrixMultiply(MatrixScale(tree.scale, tree.scale, tree.scale), MatrixRotateY(tree.yawDegrees * DEG2RAD)),
            MatrixTranslate(tree.x, 0.0f, tree.z));
        models_.add(variant.model, toWorld);
        modelledTiles_[static_cast<std::size_t>(tree.tileZ) * static_cast<std::size_t>(world.width()) +
                       static_cast<std::size_t>(tree.tileX)] = true;
    }
    for (std::size_t i = 0; i < boxTrees.size(); ++i) {
        if (boxTrees[i] > 0) {
            std::cerr << "Trees: drawing " << boxTrees[i] << " '" << data.variants[i].name
                      << "' tree(s) as box trees instead.\n";
        }
    }
    models_.finish();
}

} // namespace platform
