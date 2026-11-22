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
#include <stdexcept>
#include <string>
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

Game::Game(World world, std::vector<Npc> npcs)
    : world_(std::move(world)),
      player_{world_.playerStart(), data::kPlayerHalfSize, data::kPlayerHeight,
              data::kPlayerWalkSpeed},
      npcs_(std::move(npcs)) {
    // Check NPC placement once, up front, so a bad data file gives a clear
    // message instead of an NPC stuck in a wall or trapping the player.
    const int startX = static_cast<int>(player_.position.x);
    const int startZ = static_cast<int>(player_.position.z);
    for (std::size_t i = 0; i < npcs_.size(); ++i) {
        const Npc& npc = npcs_[i];
        const int x = npc.tileX(), z = npc.tileZ();
        const std::string where = "NPC '" + npc.name + "' at tile " + std::to_string(x) + ", " +
                                  std::to_string(z);

        if (npc.position.x < 0.0f || npc.position.z < 0.0f || x >= world_.width() ||
            z >= world_.depth()) {
            throw std::runtime_error(where + " is outside the world.");
        }
        if (isSolid(world_.tileAt(x, z))) {
            throw std::runtime_error(where + " stands on a tile that blocks movement.");
        }
        if (x == startX && z == startZ) {
            throw std::runtime_error(where + " stands on the player's start tile.");
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (npcs_[j].tileX() == x && npcs_[j].tileZ() == z) {
                throw std::runtime_error(where + " shares a tile with '" + npcs_[j].name + "'.");
            }
        }
    }
}

const Npc* Game::nearestNpcWithin(Vec3 point, float range) const {
    const Npc* nearest = nullptr;
    float nearestDistance = range;
    for (const Npc& npc : npcs_) {
        const float distance = lengthXZ(npc.position - point);
        // <= so an NPC exactly at `range` still counts.
        if (distance <= nearestDistance) {
            nearest = &npc;
            nearestDistance = distance;
        }
    }
    return nearest;
}

const Npc* Game::npcInReach() const {
    return nearestNpcWithin(player_.position, data::kTalkRange);
}

void Game::interact() {
    if (talkingTo_) {
        talkingTo_.reset(); // E again closes the dialogue
        return;
    }
    if (const Npc* npc = npcInReach()) {
        talkingTo_ = static_cast<std::size_t>(npc - npcs_.data());
    }
}

const Npc* Game::activeDialogue() const {
    return talkingTo_ ? &npcs_[*talkingTo_] : nullptr;
}

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

    // Walking out of talking range ends the conversation.
    if (talkingTo_ &&
        lengthXZ(npcs_[*talkingTo_].position - player_.position) > data::kTalkRange) {
        talkingTo_.reset();
    }
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
            if (isBlocked(x, z)) {
                return true;
            }
        }
    }
    return false;
}

bool Game::isBlocked(int x, int z) const {
    if (isSolid(world_.tileAt(x, z))) {
        return true;
    }
    // NPCs block the whole tile they stand on, like trees and stalls. This
    // keeps collision on the tile grid, so the same snap-to-edge logic works.
    for (const Npc& npc : npcs_) {
        if (npc.tileX() == x && npc.tileZ() == z) {
            return true;
        }
    }
    return false;
}

} // namespace game
