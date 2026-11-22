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

#include <array>

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
// second while an arrow key is held.
constexpr float kMouseSensitivity = 0.004f;
constexpr float kCameraKeyTurnDegPerSec = 120.0f;

// --- World shapes ------------------------------------------------------------

// Ground tiles are slabs whose top surface is at y = 0.
constexpr float kGroundThickness = 0.5f;

// Water is drawn this far below the ground, so it looks like a sunken pool.
constexpr float kWaterDepth = 0.15f;

// Walls are boxes extruded upwards from the ground.
constexpr float kWallHeight = 2.0f;

// Buildings are boxes too, a bit taller than walls. Doors are drawn where a
// path or the plaza touches a building's side.
constexpr float kBuildingHeight = 2.6f;
constexpr float kDoorWidth = 0.5f;
constexpr float kDoorHeight = 1.3f;

// Without lighting, every face of a box would be the same color. Vertical
// faces are darkened by these factors (east/west and north/south) so the
// shape of walls, buildings and props reads clearly. 1.0 = no change.
constexpr float kShadeFacingX = 0.88f;
constexpr float kShadeFacingZ = 0.76f;

// --- Colors ------------------------------------------------------------------

constexpr Color kGrassColor = {86, 140, 72, 255};
constexpr Color kPathColor = {164, 132, 90, 255};
constexpr Color kWaterColor = {54, 104, 168, 255};
constexpr Color kWallColor = {120, 114, 124, 255};
constexpr Color kWallTopColor = {168, 162, 172, 255}; // lighter cap so wall shapes read clearly
constexpr Color kWallEdgeColor = {60, 56, 64, 255};
constexpr Color kTileEdgeColor = {0, 0, 0, 40}; // faint grid on the ground
constexpr Color kPlazaColor = {178, 170, 152, 255};

// Each building gets one wall color and one roof color, picked in turn from
// these lists so neighbouring buildings look different.
constexpr std::array<Color, 4> kBuildingWallColors = {{
    {222, 206, 170, 255}, // cream plaster
    {196, 160, 120, 255}, // ochre
    {204, 190, 180, 255}, // pale stone
    {170, 128, 100, 255}, // brick-ish
}};
constexpr std::array<Color, 3> kBuildingRoofColors = {{
    {150, 70, 56, 255},  // red tiles
    {96, 92, 104, 255},  // slate
    {118, 86, 58, 255},  // thatch brown
}};
constexpr Color kBuildingEdgeColor = {50, 40, 36, 255};
constexpr Color kDoorColor = {84, 54, 34, 255};

// Props (placeholder shapes).
constexpr Color kTrunkColor = {104, 72, 46, 255};
constexpr Color kLeafColor = {52, 112, 58, 255};
constexpr Color kLeafTopColor = {70, 136, 70, 255};
constexpr Color kStallWoodColor = {140, 100, 64, 255};
constexpr std::array<Color, 3> kAwningColors = {{
    {196, 68, 60, 255},  // red
    {64, 110, 180, 255}, // blue
    {222, 178, 64, 255}, // yellow
}};
constexpr Color kStoneColor = {150, 146, 140, 255};
constexpr Color kWellWaterColor = {30, 54, 90, 255};
constexpr Color kPropEdgeColor = {0, 0, 0, 70};

constexpr Color kPlayerColor = {232, 196, 92, 255};
constexpr Color kPlayerNoseColor = {60, 40, 25, 255}; // shows which way the player faces

constexpr Color kSkyColor = {150, 190, 230, 255};
constexpr Color kHudTextColor = {20, 24, 32, 255};

// --- NPC name labels ----------------------------------------------------------

// Names float above NPCs closer than this (tiles), so the far side of town
// does not fill the screen with text.
constexpr float kNameLabelMaxDistance = 14.0f;
constexpr int kNameLabelFontSize = 18;
constexpr Color kLabelTextColor = {245, 245, 245, 255};
constexpr Color kLabelBackColor = {0, 0, 0, 140};

// --- Dialogue box -------------------------------------------------------------

constexpr int kDialogueWidth = 760;      // pixels
constexpr int kDialogueBottomGap = 40;   // from the bottom of the window
constexpr int kDialoguePadding = 18;
constexpr int kDialogueNameFontSize = 24;
constexpr int kDialogueTextFontSize = 22;
constexpr Color kDialogueBackColor = {16, 18, 24, 215};
constexpr Color kDialogueNameColor = {240, 204, 110, 255};
constexpr Color kDialogueTextColor = {240, 240, 240, 255};
constexpr Color kDialogueHintColor = {170, 170, 170, 255};

} // namespace platform::render
