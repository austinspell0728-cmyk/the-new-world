// Vec3.h
//
// A tiny 3D vector type used by the game layer.
//
// Coordinate convention (shared by the whole project):
//   X  east/west across the ground   (map column)
//   Z  north/south across the ground (map row; +Z is "down" the text map)
//   Y  height, pointing up            (0 = ground level)
//
// The ground is the X/Z plane. Collision is still 2D on that plane, so most
// helpers below have an "XZ" version that ignores height.
//
// The game layer deliberately does NOT use raylib's Vector3. Keeping our own
// type means world rules, movement and collision can be compiled and tested
// without any graphics library present. The renderer converts Vec3 to
// raylib types at the boundary.

#pragma once

#include <cmath>

namespace game {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 v, float s) { return {v.x * s, v.y * s, v.z * s}; }

// Distance across the ground, ignoring height.
inline float lengthXZ(Vec3 v) { return std::sqrt(v.x * v.x + v.z * v.z); }

// Same ground direction with length 1 and no height component.
// A zero vector stays zero, so "no input" never turns into NaN.
inline Vec3 normalizedXZ(Vec3 v) {
    const float len = lengthXZ(v);
    if (len <= 0.0f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v.x / len, 0.0f, v.z / len};
}

} // namespace game
