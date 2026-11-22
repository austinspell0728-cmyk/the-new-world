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
#include <sstream>
#include <string>
#include <vector>

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

Renderer::Renderer(const game::World& world, const BuildingMap& buildings, const std::vector<PlacedBuilding>& placed,
                   const std::string& buildingAssetDir, const TreeData& treeData,
                   const std::vector<TreePlacement>& trees, const std::string& treeAssetDir)
    // The model drawers are built first (declared first): they decide which
    // buildings and trees the box mesh should leave out.
    : buildingModels_(world, buildings, placed, buildingAssetDir),
      treeModels_(world, treeData, trees, treeAssetDir),
      worldMesh_(world, buildings, buildingModels_.modelled(), treeModels_.modelledTiles()) {}

void Renderer::draw(const game::Game& game, const CameraRig& cameraRig, const HudInfo& hud) const {
    BeginDrawing();
    ClearBackground(render::kSkyColor);

    // Everything between BeginMode3D and EndMode3D is drawn in world space
    // and seen through the camera.
    BeginMode3D(cameraRig.camera());
    worldMesh_.draw();
    buildingModels_.draw();
    treeModels_.draw();
    for (const game::Npc& npc : game.npcs()) {
        drawCharacter(npc.position, npc.halfSize, npc.height, npc.facingYaw, toColor(npc.color));
    }
    const game::Player& player = game.player();
    drawCharacter(player.position, player.halfSize, player.height, player.facingYaw,
                  render::kPlayerColor);
    EndMode3D();

    // 2D overlays are drawn after EndMode3D so they stay flat on the screen.
    drawNameLabels(game, cameraRig.camera());
    drawHud(game, hud);

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

void Renderer::drawHud(const game::Game& game, const HudInfo& hud) const {
    // Help text on a light panel, so it stays readable over dark roofs. It
    // describes whichever device was used last.
    DrawRectangle(0, 0, 820, 64, Color{255, 255, 255, 150});
    const game::Vec3 pos = game.player().position;
    const std::string help =
        hud.gamepadHints ? "Move: " + hud.moveStick + "   Camera: " + hud.cameraStick + "   Talk: " + hud.talkButton +
                               "   Quit: " + hud.quitButton
                         : "Move: WASD   Camera: mouse, arrows   Talk: E   Free mouse: Tab   Quit: Esc";
    DrawText(help.c_str(), 12, 12, 20, render::kHudTextColor);
    DrawText(TextFormat("Tile: x %d, z %d", static_cast<int>(pos.x), static_cast<int>(pos.z)), 12,
             38, 20, render::kHudTextColor);
    if (!hud.gamepadName.empty()) {
        DrawText(("gamepad: " + hud.gamepadName).c_str(), 200, 38, 20, render::kHudTextColor);
    }
    DrawFPS(render::kWindowWidth - 100, 12);

    // While talking, show the dialogue box; otherwise, hint when someone is
    // close enough to talk to.
    if (const game::Npc* speaker = game.activeDialogue()) {
        drawDialogue(*speaker, hud.gamepadHints ? hud.talkButton : "E");
    } else if (const game::Npc* npc = game.npcInReach()) {
        const std::string prompt = "Press " + (hud.gamepadHints ? hud.talkButton : std::string("E")) +
                                   " to talk to " + npc->name;
        drawLabel(prompt.c_str(), render::kWindowWidth / 2,
                  render::kWindowHeight - render::kDialogueBottomGap - 30, 22);
    }
}

void Renderer::drawDialogue(const game::Npc& speaker, const std::string& closeButton) const {
    using namespace render;
    const int textWidth = kDialogueWidth - kDialoguePadding * 2;

    // Split the line into rows that fit the box, breaking between words.
    std::vector<std::string> rows;
    std::string current;
    std::istringstream words(speaker.line);
    std::string word;
    while (words >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && MeasureText(candidate.c_str(), kDialogueTextFontSize) > textWidth) {
            rows.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
    }
    if (!current.empty()) {
        rows.push_back(current);
    }

    // Box size grows with the number of rows.
    const int rowHeight = kDialogueTextFontSize + 6;
    const int height = kDialoguePadding * 2 + kDialogueNameFontSize + 10 +
                       static_cast<int>(rows.size()) * rowHeight + 24;
    const int left = (kWindowWidth - kDialogueWidth) / 2;
    const int top = kWindowHeight - kDialogueBottomGap - height;
    DrawRectangle(left, top, kDialogueWidth, height, kDialogueBackColor);

    int y = top + kDialoguePadding;
    DrawText(speaker.name.c_str(), left + kDialoguePadding, y, kDialogueNameFontSize,
             kDialogueNameColor);
    y += kDialogueNameFontSize + 10;
    for (const std::string& row : rows) {
        DrawText(row.c_str(), left + kDialoguePadding, y, kDialogueTextFontSize, kDialogueTextColor);
        y += rowHeight;
    }
    DrawText((closeButton + ": close").c_str(), left + kDialoguePadding, y + 4, 18, kDialogueHintColor);
}

} // namespace platform
