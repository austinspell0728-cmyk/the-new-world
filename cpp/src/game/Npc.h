// Npc.h
//
// A non-player character. For now NPCs just stand in one place and have one
// line to say. Their data comes from a text file loaded at runtime (see
// NpcData.h and data/npcs.txt), not from code.

#pragma once

#include <cstdint>
#include <string>

#include "Vec3.h"

namespace game {

struct Npc {
    std::string name;
    Vec3 position;          // Centre of their feet, in tiles (always a tile centre for now).
    float facingYaw = 0.0f; // Same convention as Player::facingYaw.
    float halfSize = 0.0f;  // Half the width of their footprint.
    float height = 0.0f;    // How tall they stand, in tiles.

    // Body color as 0xRRGGBB. The game never uses it; it is carried along
    // as data so each NPC's look lives in the data file with the rest of
    // them. The renderer turns it into a real color.
    std::uint32_t color = 0xFFFFFF;

    std::string line;       // What they say when the player talks to them.

    // The tile they stand on.
    int tileX() const { return static_cast<int>(position.x); }
    int tileZ() const { return static_cast<int>(position.z); }
};

} // namespace game
