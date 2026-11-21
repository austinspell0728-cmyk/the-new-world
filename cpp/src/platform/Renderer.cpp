// Renderer.cpp
//
// World units (tiles) are converted to pixels here and only here, by
// multiplying by kPixelsPerTile.

#include "Renderer.h"

#include <algorithm>

#include "RenderConfig.h"

namespace platform {

namespace {

// Picks the color for a tile type. Purely visual: whether a tile blocks
// movement is decided by game::isSolid, not by its color.
Color colorFor(game::Tile tile) {
    switch (tile) {
    case game::Tile::Grass: return render::kGrassColor;
    case game::Tile::Path: return render::kPathColor;
    case game::Tile::Water: return render::kWaterColor;
    case game::Tile::Wall: return render::kWallColor;
    }
    return MAGENTA; // Bright "missing" color makes a forgotten tile type obvious.
}

// Converts a world-space point to pixel space (before the camera is applied).
Vector2 toPixels(game::Vec2 world) {
    return {world.x * render::kPixelsPerTile, world.y * render::kPixelsPerTile};
}

// Keeps a camera centre between `low` and `high`. If the map is smaller
// than the screen on this axis, centre the map instead.
float clampCameraAxis(float value, float low, float high) {
    if (low > high) {
        return (low + high) / 2.0f;
    }
    return std::clamp(value, low, high);
}

} // namespace

Renderer::Renderer() {
    // offset: where on the screen the camera's target appears (screen centre).
    camera_.offset = {render::kWindowWidth / 2.0f, render::kWindowHeight / 2.0f};
    camera_.rotation = 0.0f;
    camera_.zoom = 1.0f;
}

void Renderer::draw(const game::Game& game) {
    updateCamera(game);

    BeginDrawing();
    ClearBackground(render::kBackgroundColor);

    // Everything between BeginMode2D and EndMode2D is drawn in world pixel
    // coordinates and moved by the camera, so it scrolls as the player walks.
    BeginMode2D(camera_);
    drawWorld(game.world());
    drawPlayer(game.player());
    EndMode2D();

    // The HUD is drawn after EndMode2D so it stays fixed on the screen.
    drawHud(game);

    EndDrawing();
}

void Renderer::updateCamera(const game::Game& game) {
    const Vector2 playerPixels = toPixels(game.player().position);
    const float mapWidth = game.world().width() * render::kPixelsPerTile;
    const float mapHeight = game.world().height() * render::kPixelsPerTile;
    const float halfScreenW = camera_.offset.x / camera_.zoom;
    const float halfScreenH = camera_.offset.y / camera_.zoom;

    // Follow the player, but stop scrolling at the map edges so the empty
    // space beyond the map is not shown.
    camera_.target.x = clampCameraAxis(playerPixels.x, halfScreenW, mapWidth - halfScreenW);
    camera_.target.y = clampCameraAxis(playerPixels.y, halfScreenH, mapHeight - halfScreenH);
}

void Renderer::drawWorld(const game::World& world) const {
    const float size = render::kPixelsPerTile;

    for (int row = 0; row < world.height(); ++row) {
        for (int col = 0; col < world.width(); ++col) {
            const Rectangle cell{col * size, row * size, size, size};
            DrawRectangleRec(cell, colorFor(world.tileAt(col, row)));
            DrawRectangleLinesEx(cell, 1.0f, render::kGridLineColor);
        }
    }
}

void Renderer::drawPlayer(const game::Player& player) const {
    const Vector2 centre = toPixels(player.position);
    const float half = player.halfSize * render::kPixelsPerTile;

    // The drawn square matches the collision box exactly, so what you see
    // is what collides.
    const Rectangle body{centre.x - half, centre.y - half, half * 2.0f, half * 2.0f};
    DrawRectangleRec(body, render::kPlayerColor);
    DrawRectangleLinesEx(body, 2.0f, render::kPlayerOutline);

    // A small dot on the side the player faces.
    Vector2 marker = centre;
    const float reach = half * 0.6f;
    switch (player.facing) {
    case game::Facing::Up: marker.y -= reach; break;
    case game::Facing::Down: marker.y += reach; break;
    case game::Facing::Left: marker.x -= reach; break;
    case game::Facing::Right: marker.x += reach; break;
    }
    DrawCircleV(marker, half * 0.25f, render::kFacingMarkerColor);
}

void Renderer::drawHud(const game::Game& game) const {
    const game::Vec2 pos = game.player().position;
    DrawText("Move: WASD / arrow keys    Quit: Esc", 12, 12, 20, render::kHudTextColor);
    DrawText(TextFormat("Tile: %d, %d", static_cast<int>(pos.x), static_cast<int>(pos.y)), 12, 38,
             20, render::kHudTextColor);
    DrawFPS(render::kWindowWidth - 100, 12);
}

} // namespace platform
