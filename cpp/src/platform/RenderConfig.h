// RenderConfig.h
//
// Presentation settings: window, camera, placeholder 3D shapes and colors.
//
// These only change how the game LOOKS. Changing anything here must never
// change how the game plays; gameplay numbers live in game/GameData.h.
//
// Distances are in world units (1 unit = 1 tile), the same units the game
// uses, so a wall height of 2.0 means "two tiles tall".

#pragma once

#include "raylib.h"

namespace platform::render {

// --- Window ------------------------------------------------------------------

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr const char* kWindowTitle = "The New World - C++ prototype";
constexpr int kTargetFps = 60;

// --- Camera ------------------------------------------------------------------

// Vertical field of view, in degrees.
constexpr float kCameraFovY = 55.0f;

// How far the camera sits from the point it looks at, in tiles.
constexpr float kCameraDistance = 7.0f;

// The camera looks at this height above the player's feet (roughly the
// shoulders), so the player sits a little below the centre of the screen.
constexpr float kCameraLookHeight = 1.3f;

// Pitch is the angle the camera looks DOWN at the player, in degrees.
// The clamp stops the camera going under the ground or flipping over the top.
constexpr float kCameraStartPitchDeg = 35.0f;
constexpr float kCameraMinPitchDeg = 5.0f;
constexpr float kCameraMaxPitchDeg = 75.0f;

// Starting yaw: which way the camera faces across the ground, measured like
// Player::facingYaw (0 = facing +Z / south, 180 = facing -Z / north).
// 180 means the camera starts south of the player looking north, so the
// map appears the same way round as the text map in GameData.h.
constexpr float kCameraStartYawDeg = 180.0f;

// Orbit speed: radians turned per pixel of mouse movement, and degrees per
// second while Q/E or an arrow key is held.
constexpr float kMouseSensitivity = 0.004f;
constexpr float kCameraKeyTurnDegPerSec = 120.0f;

// --- World shapes ------------------------------------------------------------

// Ground tiles are slabs whose top surface is at y = 0.
constexpr float kGroundThickness = 0.5f;

// Water is drawn this far below the ground, so it looks like a sunken pool.
constexpr float kWaterDepth = 0.15f;

// Walls are boxes extruded upwards from the ground.
constexpr float kWallHeight = 2.0f;

// --- Colors ------------------------------------------------------------------

constexpr Color kGrassColor = {86, 140, 72, 255};
constexpr Color kPathColor = {164, 132, 90, 255};
constexpr Color kWaterColor = {54, 104, 168, 255};
constexpr Color kWallColor = {120, 114, 124, 255};
constexpr Color kWallTopColor = {168, 162, 172, 255}; // lighter cap so wall shapes read clearly
constexpr Color kWallEdgeColor = {60, 56, 64, 255};
constexpr Color kTileEdgeColor = {0, 0, 0, 40}; // faint grid on the ground

constexpr Color kPlayerColor = {232, 196, 92, 255};
constexpr Color kPlayerNoseColor = {60, 40, 25, 255}; // shows which way the player faces

constexpr Color kSkyColor = {150, 190, 230, 255};
constexpr Color kHudTextColor = {20, 24, 32, 255};

} // namespace platform::render
