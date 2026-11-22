// TreeData.h
//
// Reads data/trees.txt: the tree models the town can use, and the rules
// for picking one per tree tile. Presentation data (model files), so it
// lives in platform/, but plain C++ with NO raylib, so the tests check it
// without any assets. Same format and error style as data/buildings.txt.
//
// Example (see data/trees.txt for the real file):
//
//     # comment lines start with '#'
//     [tree]
//     name   = oak 1
//     model  = CommonTree_1.gltf
//     scale  = 0.7
//     trunk  = 0, 0
//     weight = 2
//
//     [rules]
//     scale_variation = 0.15
//     seed            = 1
//
// Tree keys: model is looked up in assets/nature/. scale is the base size
// (the kit's trees are 7-9 m tall at scale 1). trunk is where the trunk
// stands in the model (x, z in model units), so it can be moved onto the
// tile centre. weight is optional (default 1): how often this tree is
// picked compared with the others.
//
// [rules] is optional and may appear once. scale_variation (default 0.15)
// is how much each tree's size may differ from its base scale, as a
// fraction (0.15 = up to 15% smaller or larger, at most 0.5). seed
// (default 0) changes which tree every tile gets, without changing how
// many of each.
//
// Every tree key is required except weight. With no [tree] at all every
// tree is drawn as the old box tree. Unknown keys, repeated keys and bad
// values are errors, reported as "file:line: problem".
// Comments must be on their own line.

#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace platform {

struct TreeVariant {
    std::string name;
    std::string model;
    float scale = 1.0f;
    float trunkX = 0.0f; // trunk position in the model, model units
    float trunkZ = 0.0f;
    int weight = 1;
    int line = 0; // line of its [tree], for error messages
};

struct TreeRules {
    float scaleVariation = 0.15f;
    std::uint32_t seed = 0;
};

struct TreeData {
    std::string source; // file name, for error messages
    std::vector<TreeVariant> variants;
    TreeRules rules;

    // Lookup by name. nullptr if there is no such tree.
    const TreeVariant* findVariant(const std::string& name) const;
};

// Parses `in`. `sourceName` is used in error messages.
// Throws std::runtime_error ("source:line: problem") on bad input.
TreeData parseTrees(std::istream& in, const std::string& sourceName);

// Opens `path` and parses it. Throws std::runtime_error if the file cannot
// be opened or is invalid.
TreeData loadTreeFile(const std::string& path);

} // namespace platform
