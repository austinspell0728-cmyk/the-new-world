// TreeLayout.cpp

#include "TreeLayout.h"

#include <cmath>

namespace platform {

namespace {

// A standard 32-bit integer mixer ("lowbias32"): small changes in the
// input change about half the output bits.
std::uint32_t mix(std::uint32_t h) {
    h ^= h >> 16;
    h *= 0x7feb352dU;
    h ^= h >> 15;
    h *= 0x846ca68bU;
    h ^= h >> 16;
    return h;
}

// A number from 0 up to (not including) 1, from the top 24 bits of `h`.
float unit(std::uint32_t h) { return static_cast<float>(h >> 8) * (1.0f / 16777216.0f); }

// Separate streams for the three choices made per tile.
constexpr std::uint32_t kVariantStream = 1;
constexpr std::uint32_t kTurnStream = 2;
constexpr std::uint32_t kSizeStream = 3;

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

} // namespace

std::uint32_t tileHash(int x, int z, std::uint32_t seed, std::uint32_t stream) {
    // Each input is spread by a different odd constant before mixing, so
    // (x, z) and (z, x) give unrelated results.
    std::uint32_t h = mix(static_cast<std::uint32_t>(x) * 0x9E3779B1U);
    h = mix(h ^ (static_cast<std::uint32_t>(z) * 0x85EBCA77U));
    h = mix(h ^ (seed * 0xC2B2AE3DU));
    return mix(h ^ (stream * 0x27D4EB2FU));
}

TreePlacement placeTree(int x, int z, const TreeData& data) {
    TreePlacement tree;
    tree.tileX = x;
    tree.tileZ = z;
    const std::uint32_t seed = data.rules.seed;

    // Variant: a weighted pick. With weights 2, 2, 1 the first two are each
    // picked twice as often as the third.
    int totalWeight = 0;
    for (const TreeVariant& v : data.variants) {
        totalWeight += v.weight;
    }
    int pick = static_cast<int>(tileHash(x, z, seed, kVariantStream) % static_cast<std::uint32_t>(totalWeight));
    for (std::size_t i = 0; i < data.variants.size(); ++i) {
        pick -= data.variants[i].weight;
        if (pick < 0) {
            tree.variant = i;
            break;
        }
    }
    const TreeVariant& variant = data.variants[tree.variant];

    // Turn: any angle. Size: the base scale, up to +-scaleVariation.
    tree.yawDegrees = unit(tileHash(x, z, seed, kTurnStream)) * 360.0f;
    const float sizeChange = (unit(tileHash(x, z, seed, kSizeStream)) * 2.0f - 1.0f) * data.rules.scaleVariation;
    tree.scale = variant.scale * (1.0f + sizeChange);

    // Move the model so its trunk (turned and scaled with it) lands on the
    // tile centre. The turn uses the same convention as raylib's
    // MatrixRotateY: (x, z) -> (x cos + z sin, -x sin + z cos).
    const float yaw = tree.yawDegrees * kDegToRad;
    const float tx = variant.trunkX * tree.scale, tz = variant.trunkZ * tree.scale;
    const float trunkOffsetX = tx * std::cos(yaw) + tz * std::sin(yaw);
    const float trunkOffsetZ = -tx * std::sin(yaw) + tz * std::cos(yaw);
    tree.x = static_cast<float>(x) + 0.5f - trunkOffsetX;
    tree.z = static_cast<float>(z) + 0.5f - trunkOffsetZ;
    return tree;
}

std::vector<TreePlacement> placeTrees(const game::World& world, const TreeData& data) {
    std::vector<TreePlacement> trees;
    if (data.variants.empty()) {
        return trees;
    }
    for (int z = 0; z < world.depth(); ++z) {
        for (int x = 0; x < world.width(); ++x) {
            if (world.tileAt(x, z) == game::Tile::Tree) {
                trees.push_back(placeTree(x, z, data));
            }
        }
    }
    return trees;
}

} // namespace platform
