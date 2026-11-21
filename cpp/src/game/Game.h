// Game.h
//
// The game simulation: the world, the player, and the rules that move them.
//
// This header has no raylib dependency. The main loop feeds it a
// PlayerInput each step and the renderer reads its state afterwards:
//
//     input (platform)  ->  Game::step (rules)  ->  Renderer (drawing)

#pragma once

#include "Vec2.h"
#include "World.h"

namespace game {

// What the player is asking to do this step, already translated from
// physical keys. Each axis is -1, 0 or +1 (x: left/right, y: up/down).
// Keeping this separate from the keyboard lets us later drive the player
// from a gamepad, an AI, a replay file or a test.
struct PlayerInput {
    int moveX = 0;
    int moveY = 0;
};

// Which way the player is facing; the renderer uses it to draw a marker.
enum class Facing { Up, Down, Left, Right };

struct Player {
    Vec2 position;   // Centre of the player, in world units (tiles).
    float halfSize;  // Half the width of the square collision box.
    float speed;     // Tiles per second.
    Facing facing = Facing::Down;
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
    // Moves the player along one axis, stopping at solid tiles.
    void moveAlongX(float distance);
    void moveAlongY(float distance);

    // True if a square box centred at `centre` overlaps any solid tile.
    bool overlapsSolid(Vec2 centre, float halfSize) const;

    World world_;
    Player player_;
};

} // namespace game
