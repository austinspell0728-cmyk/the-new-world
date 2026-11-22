// Game.h
//
// The game simulation: the world, the player, and the rules that move them.
//
// This header has no raylib dependency. The main loop feeds it a
// PlayerInput each step and the renderer reads its state afterwards:
//
//     input (platform)  ->  Game::step (rules)  ->  Renderer (drawing)

#pragma once

#include "Vec3.h"
#include "World.h"

namespace game {

// The direction the player wants to walk this step, in WORLD space on the
// ground plane (+X east, +Z south; see Vec3.h).
//
// The game does not know about cameras or keys. The platform layer turns
// "W pressed while the camera faces north-east" into a world direction
// before it gets here, which keeps these rules testable with plain numbers
// and lets a gamepad, an AI or a replay drive the player the same way.
//
// Any length is accepted: the direction is normalized, so the player always
// walks at full speed, and (0, 0) means stand still.
struct PlayerInput {
    float moveX = 0.0f;
    float moveZ = 0.0f;
};

struct Player {
    Vec3 position;   // Centre of the player's feet, in tiles. y is height (0 = ground).
    float halfSize;  // Half the width of the square footprint on the ground.
    float height;    // How tall the player stands, in tiles.
    float speed;     // Tiles per second.

    // Direction the player faces, as an angle on the ground plane in radians.
    // 0 faces +Z (south), pi/2 faces +X (east); i.e. yaw = atan2(dirX, dirZ).
    // Kept when the player stops, so they keep facing where they last walked.
    float facingYaw = 0.0f;
    bool moving = false;
};

class Game {
public:
    explicit Game(World world);

    // Advances the simulation by exactly one fixed time step.
    void step(const PlayerInput& input, float dt);

    const World& world() const { return world_; }
    const Player& player() const { return player_; }

private:
    // Moves the player along one ground axis, stopping at solid tiles.
    void moveAlongX(float distance);
    void moveAlongZ(float distance);

    // True if a square footprint centred at `centre` overlaps any solid tile.
    bool overlapsSolid(Vec3 centre, float halfSize) const;

    World world_;
    Player player_;
};

} // namespace game
