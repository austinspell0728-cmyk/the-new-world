// Renderer.h
//
// Draws the current game state with raylib.
//
// The renderer only READS the game; it never changes positions or applies
// rules. If something looks wrong on screen, the cause is either here
// (presentation) or in game/ (rules), never a mix of both.

#pragma once

#include "raylib.h"

#include "../game/Game.h"

namespace platform {

class Renderer {
public:
    Renderer();

    // Draws one complete frame: world, player and on-screen help text.
    void draw(const game::Game& game);

private:
    // Points the camera at the player, keeping it inside the map edges.
    void updateCamera(const game::Game& game);

    void drawWorld(const game::World& world) const;
    void drawPlayer(const game::Player& player) const;
    void drawHud(const game::Game& game) const;

    Camera2D camera_{};
};

} // namespace platform
