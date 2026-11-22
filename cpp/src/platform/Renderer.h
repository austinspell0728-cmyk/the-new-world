// Renderer.h
//
// Draws the current game state in 3D with raylib.
//
// The renderer only READS the game; it never changes positions or applies
// rules. If something looks wrong on screen, the cause is either here
// (presentation) or in game/ (rules), never a mix of both.

#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "../game/Game.h"
#include "BuildingModels.h"
#include "TreeModels.h"
#include "CameraRig.h"
#include "WorldMesh.h"

namespace platform {

class Renderer {
public:
    // Prepares GPU data for `world`: the static map mesh, the buildings in
    // `placed` assembled from the kit pieces in `buildingAssetDir`, and the
    // `trees` from the models in `treeAssetDir` (folders end in '/').
    // Buildings or trees whose models fail to load stay boxes. The window
    // must already be open.
    Renderer(const game::World& world, const BuildingMap& buildings, const std::vector<PlacedBuilding>& placed,
             const std::string& buildingAssetDir, const TreeData& treeData,
             const std::vector<TreePlacement>& trees, const std::string& treeAssetDir);

    // Draws one complete frame, seen through `cameraRig`: world, player,
    // NPCs, their name labels and on-screen help text.
    void draw(const game::Game& game, const CameraRig& cameraRig) const;

    // Meshes drawn per frame for the kit buildings and trees (shown by
    // --benchmark).
    int buildingDrawCount() const { return buildingModels_.drawCount(); }
    int treeDrawCount() const { return treeModels_.drawCount(); }

private:
    // A standing capsule with a small "nose" showing which way it faces.
    // Used for the player and NPCs alike.
    void drawCharacter(game::Vec3 feet, float halfSize, float height, float facingYaw,
                       Color body) const;

    void drawNameLabels(const game::Game& game, const Camera3D& camera) const;
    void drawHud(const game::Game& game) const;

    // The box at the bottom of the screen showing who is talking and their
    // line, wrapped to fit.
    void drawDialogue(const game::Npc& speaker) const;

    BuildingModels buildingModels_; // these two must stay before worldMesh_
    TreeModels treeModels_;         // (see constructor)
    WorldMesh worldMesh_;
};

} // namespace platform
