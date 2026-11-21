// RenderConfig.h
//
// Presentation settings: window size, tile pixel size and colors.
//
// These only change how the game LOOKS. Changing anything here must never
// change how the game plays; gameplay numbers live in game/GameData.h.

#pragma once

#include "raylib.h"

namespace platform::render {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr const char* kWindowTitle = "The New World - C++ prototype";
constexpr int kTargetFps = 60;

// How many screen pixels one world unit (one tile) takes up.
constexpr float kPixelsPerTile = 32.0f;

// Tile colors.
constexpr Color kGrassColor = {86, 140, 72, 255};
constexpr Color kPathColor = {164, 132, 90, 255};
constexpr Color kWaterColor = {54, 104, 168, 255};
constexpr Color kWallColor = {88, 84, 92, 255};
constexpr Color kGridLineColor = {0, 0, 0, 28};

// Player colors.
constexpr Color kPlayerColor = {232, 196, 92, 255};
constexpr Color kPlayerOutline = {40, 30, 20, 255};
constexpr Color kFacingMarkerColor = {40, 30, 20, 255};

// Background shown outside the map and behind the HUD text.
constexpr Color kBackgroundColor = {18, 18, 22, 255};
constexpr Color kHudTextColor = {235, 235, 235, 255};

} // namespace platform::render
