// Renderer.cpp
//
// Placeholder 3D art: the map and the player are simple flat-colored
// shapes. Positions come straight from the game in tiles, which raylib
// uses directly as its 3D units (1 tile = 1 unit).
//
// The static map is pre-built into a single mesh (see WorldMesh); only
// things that move are drawn shape by shape each frame.

#include "Renderer.h"

#include <cmath>

#include "RenderConfig.h"

namespace platform {

namespace {

Vector3 toRaylib(game::Vec3 v) { return {v.x, v.y, v.z}; }

} // namespace

Renderer::Renderer(const game::World& world) : worldMesh_(world) {}

void Renderer::draw(const game::Game& game, const CameraRig& cameraRig) const {
    BeginDrawing();
    ClearBackground(render::kSkyColor);

    // Everything between BeginMode3D and EndMode3D is drawn in world space
    // and seen through the camera.
    BeginMode3D(cameraRig.camera());
    worldMesh_.draw();
    drawPlayer(game.player());
    EndMode3D();

    // The HUD is drawn after EndMode3D so it stays fixed on the screen.
    drawHud(game);

    EndDrawing();
}

void Renderer::drawPlayer(const game::Player& player) const {
    // A capsule standing on the ground. Its radius matches the collision
    // footprint, so what you see is (roughly) what collides. The capsule's
    // end spheres sit `radius` in from the top and bottom.
    const float radius = player.halfSize;
    const Vector3 feet = toRaylib(player.position);
    const Vector3 bottom{feet.x, feet.y + radius, feet.z};
    const Vector3 top{feet.x, feet.y + player.height - radius, feet.z};
    DrawCapsule(bottom, top, radius, 12, 6, render::kPlayerColor);

    // A small "nose" at head height on the side the player faces.
    // facingYaw is measured from +Z towards +X, so sin gives the X part
    // and cos the Z part.
    const Vector3 nose{feet.x + std::sin(player.facingYaw) * radius,
                       feet.y + player.height * 0.8f,
                       feet.z + std::cos(player.facingYaw) * radius};
    DrawSphere(nose, radius * 0.3f, render::kPlayerNoseColor);
}

void Renderer::drawHud(const game::Game& game) const {
    const game::Vec3 pos = game.player().position;
    DrawText("Move: WASD    Camera: mouse, Q/E, arrows    Free mouse: Tab    Quit: Esc", 12, 12,
             20, render::kHudTextColor);
    DrawText(TextFormat("Tile: x %d, z %d", static_cast<int>(pos.x), static_cast<int>(pos.z)), 12,
             38, 20, render::kHudTextColor);
    DrawFPS(render::kWindowWidth - 100, 12);
}

} // namespace platform
