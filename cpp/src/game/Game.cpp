// Game.cpp
//
// Movement and collision rules.
//
// Collision approach: the player's footprint is an axis-aligned square on
// the ground (X/Z). Each step we move along X first and then along Z,
// checking after each move whether the footprint now overlaps a solid tile.
// Resolving the axes separately is what lets the player slide along a wall
// when pushing into it at an angle, instead of sticking to it.
//
// Collision is 2D for now: height (Y) is ignored because nothing can jump
// or fall yet.

#include "Game.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "GameData.h"

namespace game {

namespace {

// Small gap kept between the player and a wall after a collision, so floating
// point rounding never leaves the footprint a hair inside the wall.
constexpr float kSkin = 0.001f;

// Longest distance moved in one collision check. Must stay well below one
// tile (and below the player's size) so a fast player can never skip
// over a thin wall between two checks.
constexpr float kMaxSubstep = 0.25f;

// Splits `distance` into sub-steps no longer than kMaxSubstep.
int substepsFor(float distance) {
    return std::max(1, static_cast<int>(std::ceil(std::abs(distance) / kMaxSubstep)));
}

} // namespace

Game::Game(World world)
    : world_(std::move(world)),
      player_{world_.playerStart(), data::kPlayerHalfSize, data::kPlayerHeight,
              data::kPlayerWalkSpeed} {}

void Game::step(const PlayerInput& input, float dt) {
    // Normalizing does two jobs: diagonal movement is not ~41% faster than
    // straight movement, and an oversized input can never speed the player
    // up. NaN or infinite input is treated as "no input".
    Vec3 direction{input.moveX, 0.0f, input.moveZ};
    if (!std::isfinite(direction.x) || !std::isfinite(direction.z)) {
        direction = {};
    }
    direction = normalizedXZ(direction);

    player_.moving = (direction.x != 0.0f || direction.z != 0.0f);
    if (!player_.moving) {
        return;
    }

    // Turn to face the way we are walking (see Player::facingYaw).
    player_.facingYaw = std::atan2(direction.x, direction.z);

    const Vec3 delta = direction * (player_.speed * dt);
    moveAlongX(delta.x);
    moveAlongZ(delta.z);
}

void Game::moveAlongX(float distance) {
    const int steps = substepsFor(distance);
    const float stepDistance = distance / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        Vec3 next = player_.position;
        next.x += stepDistance;

        if (!overlapsSolid(next, player_.halfSize)) {
            player_.position = next;
            continue;
        }

        // Blocked: snap flush against the wall we ran into, then stop.
        // Moving +X, the wall's near edge is the tile column we entered;
        // moving -X, it is one past that column.
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

void Game::moveAlongZ(float distance) {
    // Same as moveAlongX, on the Z axis.
    const int steps = substepsFor(distance);
    const float stepDistance = distance / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        Vec3 next = player_.position;
        next.z += stepDistance;

        if (!overlapsSolid(next, player_.halfSize)) {
            player_.position = next;
            continue;
        }

        if (stepDistance > 0.0f) {
            const float wallEdge = std::floor(next.z + player_.halfSize);
            player_.position.z = wallEdge - player_.halfSize - kSkin;
        } else {
            const float wallEdge = std::floor(next.z - player_.halfSize) + 1.0f;
            player_.position.z = wallEdge + player_.halfSize + kSkin;
        }
        return;
    }
}

bool Game::overlapsSolid(Vec3 centre, float halfSize) const {
    // Find the range of tile cells the footprint covers. floor() turns a
    // world coordinate into the index of the tile containing it.
    const int minX = static_cast<int>(std::floor(centre.x - halfSize));
    const int maxX = static_cast<int>(std::floor(centre.x + halfSize));
    const int minZ = static_cast<int>(std::floor(centre.z - halfSize));
    const int maxZ = static_cast<int>(std::floor(centre.z + halfSize));

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            if (isSolid(world_.tileAt(x, z))) {
                return true;
            }
        }
    }
    return false;
}

} // namespace game
