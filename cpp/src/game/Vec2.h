// Vec2.h
//
// A tiny 2D vector type used by the game layer.
//
// The game layer deliberately does NOT use raylib's Vector2. Keeping our own
// type means world rules, movement and collision can be compiled, tested and
// reasoned about without any graphics library present. The renderer converts
// Vec2 to raylib types at the boundary.

#pragma once

#include <cmath>

namespace game {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 v, float s) { return {v.x * s, v.y * s}; }

// Length of the vector (distance from the origin).
inline float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// Returns a vector pointing the same way with length 1.
// A zero vector stays zero, so "no input" never turns into NaN.
inline Vec2 normalized(Vec2 v) {
    const float len = length(v);
    if (len <= 0.0f) {
        return {0.0f, 0.0f};
    }
    return {v.x / len, v.y / len};
}

} // namespace game
