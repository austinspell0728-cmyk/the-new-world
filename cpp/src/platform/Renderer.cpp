// Renderer.cpp
//
// Placeholder 3D art: every tile and the player are simple flat-colored
// shapes. Positions come straight from the game in tiles, which raylib
// uses directly as its 3D units (1 tile = 1 unit).
//
// Tile (x, z) covers the ground square from (x, z) to (x + 1, z + 1), so
// its centre is at (x + 0.5, z + 0.5).

#include "Renderer.h"

#include <cmath>

#include "RenderConfig.h"

namespace platform {

namespace {

// Picks the color for a tile type. Purely visual: whether a tile blocks
// movement is decided by game::isSolid, not by its look.
Color colorFor(game::Tile tile) {
    switch (tile) {
    case game::Tile::Grass: return render::kGrassColor;
    case game::Tile::Path: return render::kPathColor;
    case game::Tile::Water: return render::kWaterColor;
    case game::Tile::Wall: return render::kWallColor;
    }
    return MAGENTA; // Bright "missing" color makes a forgotten tile type obvious.
}

Vector3 toRaylib(game::Vec3 v) { return {v.x, v.y, v.z}; }

// Draws a box covering tile (x, z) whose top is at `topY` and whose
// bottom is at `bottomY`.
void drawTileBox(int x, int z, float bottomY, float topY, Color fill, Color edge) {
    const float height = topY - bottomY;
    const Vector3 centre{x + 0.5f, bottomY + height / 2.0f, z + 0.5f};
    DrawCube(centre, 1.0f, height, 1.0f, fill);
    DrawCubeWires(centre, 1.0f, height, 1.0f, edge);
}

} // namespace

void Renderer::draw(const game::Game& game, const CameraRig& cameraRig) const {
    BeginDrawing();
    ClearBackground(render::kSkyColor);

    // Everything between BeginMode3D and EndMode3D is drawn in world space
    // and seen through the camera.
    BeginMode3D(cameraRig.camera());
    drawWorld(game.world());
    drawPlayer(game.player());
    EndMode3D();

    // The HUD is drawn after EndMode3D so it stays fixed on the screen.
    drawHud(game);

    EndDrawing();
}

void Renderer::drawWorld(const game::World& world) const {
    const float groundBottom = -render::kGroundThickness;

    for (int z = 0; z < world.depth(); ++z) {
        for (int x = 0; x < world.width(); ++x) {
            const game::Tile tile = world.tileAt(x, z);
            const Color fill = colorFor(tile);

            switch (tile) {
            case game::Tile::Grass:
            case game::Tile::Path:
                // Flat ground slab, top surface at y = 0.
                drawTileBox(x, z, groundBottom, 0.0f, fill, render::kTileEdgeColor);
                break;
            case game::Tile::Water:
                // Slab sunk a little below the ground, so the neighbouring
                // ground tiles form a visible bank around the pool.
                drawTileBox(x, z, groundBottom, -render::kWaterDepth, fill, render::kTileEdgeColor);
                break;
            case game::Tile::Wall:
                // Box extruded up from below the ground to the wall height,
                // plus a thin lighter cap. Without lighting every face would
                // be the same color; the cap makes the wall tops stand out.
                drawTileBox(x, z, groundBottom, render::kWallHeight, fill, render::kWallEdgeColor);
                drawTileBox(x, z, render::kWallHeight, render::kWallHeight + 0.02f,
                            render::kWallTopColor, render::kWallEdgeColor);
                break;
            }
        }
    }
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
    DrawText("Move: WASD    Quit: Esc", 12, 12, 20, render::kHudTextColor);
    DrawText(TextFormat("Tile: x %d, z %d", static_cast<int>(pos.x), static_cast<int>(pos.z)), 12,
             38, 20, render::kHudTextColor);
    DrawFPS(render::kWindowWidth - 100, 12);
}

} // namespace platform
