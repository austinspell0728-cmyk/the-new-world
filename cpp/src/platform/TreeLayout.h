// TreeLayout.h
//
// Decides which tree stands on each tree tile, how it is turned and how
// big it is. Everything comes from a hash of the tile's coordinates (and
// the seed in data/trees.txt): no random state, so the same tile always
// gets the same tree, on every run and every machine. Plain C++ with NO
// raylib, so it is tested without assets. TreeModels.cpp draws the result.
//
// It never changes the game: tree tiles still block their whole tile.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../game/World.h"
#include "TreeData.h"

namespace platform {

// A well-mixed 32-bit number from a tile, the seed and a `stream` number.
// Different streams give unrelated numbers for the same tile, so the
// variant, the turn and the size do not follow each other.
std::uint32_t tileHash(int x, int z, std::uint32_t seed, std::uint32_t stream);

// One tree in the world. To place the model: scale it by `scale`, turn it
// by yawDegrees around Y (same angles as everywhere else), then move its
// origin to (x, 0, z). The origin is shifted so that the trunk (TreeVariant
// trunkX/Z) ends up on the tile centre.
struct TreePlacement {
    int tileX = 0;
    int tileZ = 0;
    std::size_t variant = 0; // index into TreeData::variants
    float x = 0.0f;
    float z = 0.0f;
    float yawDegrees = 0.0f; // 0 up to (not including) 360
    float scale = 1.0f;
};

// The tree for tile (x, z). `data` must have at least one variant.
TreePlacement placeTree(int x, int z, const TreeData& data);

// A tree for every Tree tile of `world`, in row order. Empty if `data` has
// no variants (then every tree stays a box tree).
std::vector<TreePlacement> placeTrees(const game::World& world, const TreeData& data);

} // namespace platform
