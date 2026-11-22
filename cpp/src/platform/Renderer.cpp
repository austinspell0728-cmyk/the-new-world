// Renderer.cpp
//
// Placeholder 3D art: the map, the player and NPCs are simple flat-colored
// shapes. Positions come straight from the game in tiles, which raylib
// uses directly as its 3D units (1 tile = 1 unit).
//
// The static map is pre-built into a single mesh (see WorldMesh); only
// characters are drawn shape by shape each frame.

#include "Renderer.h"

#include <cmath>

#include "RenderConfig.h"

namespace platform {

namespace {

Vector3 toRaylib(game::Vec3 v) { return {v.x, v.y, v.z}; }

// NPC colors are stored by the game as plain 0xRRGGBB numbers.
Color toColor(std::uint32_t rgb) {
    return {static_cast<unsigned char>((rgb >> 16) & 0xFF), static_cast<unsigned char>((rgb >> 8) & 0xFF),
            static_cast<unsigned char>(rgb & 0xFF), 255};
}

// Text with a dark box behind it, centred on (centreX, top).
void drawLabel(const char* text, int centreX, int top, int fontSize) {
    const int width = MeasureText(text, fontSize);
    const int padding = 4;
    DrawRectangle(centreX - width / 2 - padding, top - padding, width + padding * 2,
                  fontSize + padding * 2, render::kLabelBackColor);
    DrawText(text, centreX - width / 2, top, fontSize, render::kLabelTextColor);
}

} // namespace

Renderer::Renderer(const game::World& world) : worldMesh_(world) {}

void Renderer::draw(const game::Game& game, const CameraRig& cameraRig) const {
    BeginDrawing();
    ClearBackground(render::kSkyColor);

    // Everything between BeginMode3D and EndMode3D is drawn in world space
    // and seen through the camera.
    BeginMode3D(cameraRig.camera());
    worldMesh_.draw();
    for (const game::Npc& npc : game.npcs()) {
        drawCharacter(npc.position, npc.halfSize, npc.height, npc.facingYaw, toColor(npc.color));
    }
    const game::Player& player = game.player();
    drawCharacter(player.position, player.halfSize, player.height, player.facingYaw,
                  render::kPlayerColor);
    EndMode3D();

    // 2D overlays are drawn after EndMode3D so they stay flat on the screen.
    drawNameLabels(game, cameraRig.camera());
    drawHud(game);

    EndDrawing();
}

void Renderer::drawCharacter(game::Vec3 feet, float halfSize, float height, float facingYaw,
                             Color body) const {
    // A capsule standing on the ground. Its radius matches the footprint
    // half-width. The capsule's end spheres sit `radius` in from the top
    // and bottom.
    const float radius = halfSize;
    const Vector3 base = toRaylib(feet);
    const Vector3 bottom{base.x, base.y + radius, base.z};
    const Vector3 top{base.x, base.y + height - radius, base.z};
    DrawCapsule(bottom, top, radius, 12, 6, body);

    // A small "nose" at head height on the side the character faces.
    // facingYaw is measured from +Z towards +X, so sin gives the X part
    // and cos the Z part.
    const Vector3 nose{base.x + std::sin(facingYaw) * radius, base.y + height * 0.8f,
                       base.z + std::cos(facingYaw) * radius};
    DrawSphere(nose, radius * 0.3f, render::kPlayerNoseColor);
}

void Renderer::drawNameLabels(const game::Game& game, const Camera3D& camera) const {
    // Which way the camera looks, to skip NPCs behind it (projecting a point
    // behind the camera onto the screen gives a meaningless position).
    const Vector3 view{camera.target.x - camera.position.x, camera.target.y - camera.position.y,
                       camera.target.z - camera.position.z};

    for (const game::Npc& npc : game.npcs()) {
        if (game::lengthXZ(npc.position - game.player().position) > render::kNameLabelMaxDistance) {
            continue;
        }
        const Vector3 head{npc.position.x, npc.position.y + npc.height + 0.35f, npc.position.z};
        const Vector3 toHead{head.x - camera.position.x, head.y - camera.position.y,
                             head.z - camera.position.z};
        if (view.x * toHead.x + view.y * toHead.y + view.z * toHead.z <= 0.0f) {
            continue;
        }
        const Vector2 screen = GetWorldToScreen(head, camera);
        drawLabel(npc.name.c_str(), static_cast<int>(screen.x), static_cast<int>(screen.y),
                  render::kNameLabelFontSize);
    }
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
