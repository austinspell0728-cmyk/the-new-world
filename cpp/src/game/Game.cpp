// Game.cpp
//
// Movement and collision rules.
//
// Collision approach: the player is an axis-aligned square. Each step we move
// along X first and then along Y, checking after each move whether the box
// now overlaps a solid tile. Resolving the axes separately is what lets the
// player slide along a wall when pushing into it diagonally, instead of
// sticking to it.

#include "Game.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "GameData.h"

namespace game {

namespace {

// Small gap kept between the player and a wall after a collision, so floating
// point rounding never leaves the box a hair inside the wall.
constexpr float kSkin = 0.001f;

// Longest distance moved in one collision check. Must stay well below one
// tile (and below the player's size) so a fast player can never skip
// over a thin wall between two checks.
constexpr float kMaxSubstep = 0.25f;

} // namespace

Game::Game(World world)
    : world_(std::move(world)),
      player_{world_.playerStart(), data::kPlayerHalfSize, data::kPlayerWalkSpeed} {}

void Game::step(const PlayerInput& input, float dt) {
    // Clamp each axis to -1..1 so a bad input can never speed the player up.
    Vec2 direction{static_cast<float>(std::clamp(input.moveX, -1, 1)),
                   static_cast<float>(std::clamp(input.moveY, -1, 1))};

    // Without this, moving diagonally would be ~41% faster than moving
    // straight, because (1, 1) is longer than (1, 0).
    direction = normalized(direction);

    player_.moving = (direction.x != 0.0f || direction.y != 0.0f);
    if (!player_.moving) {
        return;
    }

    // Facing follows the stronger axis; horizontal wins ties so diagonal
    // movement shows a consistent left/right facing.
    if (std::abs(direction.x) >= std::abs(direction.y)) {
        player_.facing = direction.x > 0.0f ? Facing::Right : Facing::Left;
    } else {
        player_.facing = direction.y > 0.0f ? Facing::Down : Facing::Up;
    }

    const Vec2 delta = direction * (player_.speed * dt);
    moveAlongX(delta.x);
    moveAlongY(delta.y);
}

void Game::moveAlongX(float distance) {
    // Split long moves into short sub-steps (see kMaxSubstep).
    const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(distance) / kMaxSubstep)));
    const float stepDistance = distance / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        Vec2 next = player_.position;
        next.x += stepDistance;

        if (!overlapsSolid(next, player_.halfSize)) {
            player_.position = next;
            continue;
        }

        // Blocked: snap flush against the wall we ran into, then stop.
        // Moving right, the wall's left edge is the tile column we entered;
        // moving left, the wall's right edge is one past that column.
        if (stepDistance > 0.0f) {
            const float wallEdge = std::floor(next.x + player_.halfSize);
            player_.position.x = wallEdge - player_.halfSize - kSkin;
        } else {
            const float wallEdge = std::floor(next.x - player_.halfSize) + 1.0f;
            player_.position.x = wallEdge + player_.halfSize + kSkin;
        }
        return;
    }
}

void Game::moveAlongY(float distance) {
    // Same as moveAlongX, on the vertical axis (+y is down the screen).
    const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(distance) / kMaxSubstep)));
    const float stepDistance = distance / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        Vec2 next = player_.position;
        next.y += stepDistance;

        if (!overlapsSolid(next, player_.halfSize)) {
            player_.position = next;
            continue;
        }

        if (stepDistance > 0.0f) {
            const float wallEdge = std::floor(next.y + player_.halfSize);
            player_.position.y = wallEdge - player_.halfSize - kSkin;
        } else {
            const float wallEdge = std::floor(next.y - player_.halfSize) + 1.0f;
            player_.position.y = wallEdge + player_.halfSize + kSkin;
        }
        return;
    }
}

bool Game::overlapsSolid(Vec2 centre, float halfSize) const {
    // Find the range of tile cells the box covers. floor() turns a world
    // coordinate into the index of the tile containing it.
    const int left = static_cast<int>(std::floor(centre.x - halfSize));
    const int right = static_cast<int>(std::floor(centre.x + halfSize));
    const int top = static_cast<int>(std::floor(centre.y - halfSize));
    const int bottom = static_cast<int>(std::floor(centre.y + halfSize));

    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            if (isSolid(world_.tileAt(col, row))) {
                return true;
            }
        }
    }
    return false;
}

} // namespace game
