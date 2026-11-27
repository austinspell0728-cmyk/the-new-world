// CameraOcclusion.cpp

#include "CameraOcclusion.h"

#include <algorithm>
#include <cmath>

#include "GameData.h"

namespace game {

namespace {

// Does tile (x, z) block the camera at height y? Looks at the tile itself
// and, for eaves and crowns, its eight neighbours.
bool tileBlocks(const World& world, int x, int z, float y) {
    switch (world.tileAt(x, z)) { // outside the map counts as Wall
    case Tile::Building: if (y < data::kOccluderBuildingTop) return true; break;
    case Tile::Tree: if (y < data::kOccluderTreeTop) return true; break;
    case Tile::Stall: if (y < data::kOccluderStallTop) return true; break;
    case Tile::Well: if (y < data::kOccluderWellTop) return true; break;
    case Tile::Wall: if (y < data::kOccluderWallTop) return true; break;
    case Tile::Grass:
    case Tile::Path:
    case Tile::Plaza:
    case Tile::Water: break;
    }

    const bool inEaves = y >= data::kOccluderEavesBottom && y < data::kOccluderEavesTop;
    const bool inCrowns = y >= data::kOccluderCrownBottom && y < data::kOccluderTreeTop;
    if (!inEaves && !inCrowns) {
        return false;
    }
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            const Tile neighbour = world.tileAt(x + dx, z + dz);
            if ((inEaves && neighbour == Tile::Building) || (inCrowns && neighbour == Tile::Tree)) {
                return true;
            }
        }
    }
    return false;
}

int tileOf(float v) { return static_cast<int>(std::floor(v)); }

bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

} // namespace

bool cameraBlockedAt(const World& world, Vec3 point) {
    if (point.y < 0.1f) {
        return true; // under the ground
    }
    return tileBlocks(world, tileOf(point.x), tileOf(point.z), point.y);
}

bool cameraClearAt(const World& world, Vec3 point, float clearance) {
    if (point.y < 0.1f) {
        return false;
    }
    // Every tile touched by the square of half-size `clearance` around the point.
    for (int z = tileOf(point.z - clearance); z <= tileOf(point.z + clearance); ++z) {
        for (int x = tileOf(point.x - clearance); x <= tileOf(point.x + clearance); ++x) {
            if (tileBlocks(world, x, z, point.y)) {
                return false;
            }
        }
    }
    return true;
}

Vec3 unblockedCameraPosition(const World& world, Vec3 lookAt, Vec3 desired) {
    if (!finite(lookAt) || !finite(desired)) {
        return finite(lookAt) ? lookAt : Vec3{};
    }
    const Vec3 line = desired - lookAt;
    const float length = std::sqrt(line.x * line.x + line.y * line.y + line.z * line.z);
    if (length < 1e-4f) {
        return desired; // camera on the look point: nothing to check
    }
    const Vec3 direction = line * (1.0f / length);
    auto at = [&](float d) { return lookAt + direction * d; };
    const float step = data::kCameraSampleStep;

    // 1. Walk out from the player until something blocks the line.
    float allowed = length;
    for (float d = step; d < length + step; d += step) {
        const float clamped = std::min(d, length);
        if (cameraBlockedAt(world, at(clamped))) {
            allowed = std::max(0.0f, clamped - step); // just before it
            break;
        }
    }

    // 2. Pull in further until the camera has room around it.
    while (allowed > 0.0f && !cameraClearAt(world, at(allowed), data::kCameraClearance)) {
        allowed = std::max(0.0f, allowed - step);
    }

    // 3. Not closer than the minimum distance, unless that point is itself
    //    inside something (e.g. the player is pressed against a wall).
    const float minimum = std::min(data::kCameraMinDistance, length);
    if (allowed < minimum) {
        bool lineFree = true;
        for (float d = step; d <= minimum; d += step) {
            lineFree = lineFree && !cameraBlockedAt(world, at(d));
        }
        if (lineFree && !cameraBlockedAt(world, at(minimum))) {
            allowed = minimum;
        }
    }
    // 4. Never on top of the look point (see kCameraNearestDistance).
    allowed = std::max(allowed, std::min(data::kCameraNearestDistance, length));
    return allowed >= length ? desired : at(allowed);
}

Vec3 CameraOcclusion::update(const World& world, Vec3 lookAt, Vec3 desired, float dt) {
    const Vec3 allowedPoint = unblockedCameraPosition(world, lookAt, desired);
    const Vec3 line = desired - lookAt;
    const float length = std::sqrt(line.x * line.x + line.y * line.y + line.z * line.z);
    if (!finite(allowedPoint) || !std::isfinite(length) || length < 1e-4f) {
        distance_ = 0.0f;
        return allowedPoint;
    }
    const Vec3 toAllowed = allowedPoint - lookAt;
    const float target = std::sqrt(toAllowed.x * toAllowed.x + toAllowed.y * toAllowed.y + toAllowed.z * toAllowed.z);

    if (distance_ < 0.0f) {
        distance_ = target; // first frame: start where the camera is allowed
    } else if (std::isfinite(dt) && dt > 0.0f) {
        // Steady rates: the whole range (normal distance to minimum) in
        // kCameraPullInSeconds going in, kCameraReleaseSeconds going out.
        const float range = std::max(length - data::kCameraMinDistance, 0.5f);
        const float maxStep = (target < distance_ ? range / data::kCameraPullInSeconds
                                                  : range / data::kCameraReleaseSeconds) * dt;
        distance_ += std::clamp(target - distance_, -maxStep, maxStep);
    }
    distance_ = std::clamp(distance_, std::min(data::kCameraNearestDistance, length), length);
    return lookAt + line * (distance_ / length);
}

} // namespace game
