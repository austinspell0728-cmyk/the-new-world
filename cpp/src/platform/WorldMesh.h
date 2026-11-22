// WorldMesh.h
//
// The static part of the world (ground, water, walls) baked into one GPU
// mesh when the game starts, instead of drawing every tile as its own cube
// every frame.
//
// Why: drawing ~800 cubes one by one meant ~800 small draw requests per
// frame plus re-sending all their vertices each time. The baked mesh is sent
// to the GPU once and drawn with a single call, and faces that can never be
// seen (between two walls, under the ground) are left out entirely.
//
// The map never changes at runtime yet, so the mesh is built once. When
// tiles can change, rebuild it (or split it into chunks and rebuild only the
// chunk that changed).

#pragma once

#include <vector>

#include "raylib.h"

#include "../game/World.h"

namespace platform {

class WorldMesh {
public:
    // Builds and uploads the mesh. The window (and so the GPU context) must
    // already be open.
    explicit WorldMesh(const game::World& world);
    ~WorldMesh();

    // Owns GPU memory, so it cannot be copied.
    WorldMesh(const WorldMesh&) = delete;
    WorldMesh& operator=(const WorldMesh&) = delete;

    // Draws the world. Call between BeginMode3D and EndMode3D.
    void draw() const;

private:
    // One outline segment (the thin lines along tile and wall edges).
    struct EdgeLine {
        Vector3 from;
        Vector3 to;
        Color color;
    };

    Model model_{};
    std::vector<EdgeLine> edges_;
};

} // namespace platform
