// Renderer.h
//
// Draws the current game state in 3D with raylib.
//
// The renderer only READS the game; it never changes positions or applies
// rules. If something looks wrong on screen, the cause is either here
// (presentation) or in game/ (rules), never a mix of both.

#pragma once

#include "raylib.h"

#include "../game/Game.h"
#include "CameraRig.h"

namespace platform {

class Renderer {
public:
    // Draws one complete frame, seen through `cameraRig`: world, player and
    // on-screen help text.
    void draw(const game::Game& game, const CameraRig& cameraRig) const;

private:
    void drawWorld(const game::World& world) const;
    void drawPlayer(const game::Player& player) const;
    void drawHud(const game::Game& game) const;
};

} // namespace platform
