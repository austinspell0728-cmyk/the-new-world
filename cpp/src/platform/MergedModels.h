// MergedModels.h
//
// Draws many copies of a few models cheaply: each model file is loaded
// ONCE, every copy is baked (already moved, turned and scaled) into big
// meshes at startup, one per material, and each frame draws just those
// meshes. Used for the kit buildings and the trees; the world never
// changes at runtime, so the meshes are built once.
//
// Usage: load() each file you need, add() each copy, finish() once, then
// draw() every frame.
//
// Like the box world, the merged meshes get a fake light: each vertex is
// darkened by which way it faces (see RenderConfig), because we draw
// without lights.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "raylib.h"

namespace platform {

class MergedModels {
public:
    // `assetDir` ends in '/'. `label` starts warning messages ("Trees").
    // With `alphaCutout`, pixels whose texture is mostly see-through are
    // not drawn at all (for leaves drawn on flat cards); otherwise
    // textures are drawn as they are.
    MergedModels(std::string assetDir, std::string label, bool alphaCutout);
    ~MergedModels();

    // Owns GPU memory, so it cannot be copied.
    MergedModels(const MergedModels&) = delete;
    MergedModels& operator=(const MergedModels&) = delete;

    // Loads `file` from the asset folder, once. Returns false (and prints
    // which file, the first time) if it is missing or cannot be loaded.
    // The window must already be open.
    bool load(const std::string& file);

    // Adds one copy of a loaded `file`, placed by `toWorld`.
    void add(const std::string& file, const Matrix& toWorld);

    // Uploads the merged meshes. Call once, after the last add().
    void finish();

    // Call between BeginMode3D and EndMode3D.
    void draw() const;

    // Number of meshes draw() sends each frame.
    int drawCount() const { return static_cast<int>(batches_.size()); }

private:
    // Triangles collected for one material until a mesh is full.
    struct MeshData {
        std::vector<float> positions;      // x, y, z
        std::vector<float> texcoords;      // u, v
        std::vector<unsigned char> colors; // r, g, b, a
        std::vector<unsigned short> indices;
        int vertexCount() const { return static_cast<int>(positions.size() / 3); }
    };

    // A loaded model file.
    struct Source {
        int model = -1;                // index into sources_, -1 = failed to load
        std::vector<std::string> keys; // merge key per raylib material index
    };

    // Everything with the same material, possibly split over several
    // meshes (one mesh holds at most 65535 vertices).
    struct Group {
        Material material{};
        std::vector<MeshData> meshes;
    };

    // One uploaded mesh and the material it is drawn with. The material
    // (and its texture) belongs to a model in sources_.
    struct Batch {
        Mesh mesh;
        Material material;
    };

    std::string assetDir_;
    std::string label_;
    bool alphaCutout_;
    Shader cutoutShader_{};
    std::map<std::string, Source> loaded_; // by file name
    std::vector<Model> sources_;
    std::map<std::string, Group> groups_;   // by material name
    std::vector<Batch> batches_;
};

} // namespace platform
