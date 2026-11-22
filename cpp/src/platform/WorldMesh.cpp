// WorldMesh.cpp
//
// How the mesh is built, tile by tile:
//   - every tile gets a flat top face at its surface height
//     (ground 0, water slightly below, walls at wall height);
//   - a side face is added only where a neighbouring surface is LOWER, so
//     the wall between two wall tiles, or between two ground tiles, is never
//     generated;
//   - colors are stored per vertex, so one mesh can hold every tile type
//     with no textures.
// The look matches the old per-tile cubes: same colors, same heights, and
// the same thin outlines along tile and wall edges.

#include "WorldMesh.h"

#include <array>
#include <cstring>
#include <set>

#include "rlgl.h"

#include "RenderConfig.h"

namespace platform {

namespace {

// --- Look of each tile type --------------------------------------------------

// Height of a tile's top surface.
float surfaceHeight(game::Tile tile) {
    switch (tile) {
    case game::Tile::Grass:
    case game::Tile::Path: return 0.0f;
    case game::Tile::Water: return -render::kWaterDepth;
    case game::Tile::Wall: return render::kWallHeight;
    }
    return 0.0f;
}

Color topColor(game::Tile tile) {
    switch (tile) {
    case game::Tile::Grass: return render::kGrassColor;
    case game::Tile::Path: return render::kPathColor;
    case game::Tile::Water: return render::kWaterColor;
    case game::Tile::Wall: return render::kWallTopColor;
    }
    return MAGENTA; // Bright "missing" color makes a forgotten tile type obvious.
}

Color sideColor(game::Tile tile) {
    return tile == game::Tile::Wall ? render::kWallColor : topColor(tile);
}

Color edgeColor(game::Tile tile) {
    return tile == game::Tile::Wall ? render::kWallEdgeColor : render::kTileEdgeColor;
}

// --- Mesh building helpers -----------------------------------------------------

Vector3 subtract(Vector3 a, Vector3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

Vector3 cross(Vector3 a, Vector3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float dot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Collects triangles in plain arrays until they are uploaded.
struct MeshBuilder {
    std::vector<float> positions;      // x, y, z per vertex
    std::vector<unsigned char> colors; // r, g, b, a per vertex

    void vertex(Vector3 p, Color c) {
        positions.insert(positions.end(), {p.x, p.y, p.z});
        colors.insert(colors.end(), {c.r, c.g, c.b, c.a});
    }

    // Adds a flat four-cornered face (corners listed in order around its
    // edge) as two triangles. The GPU skips triangles seen from behind, and
    // "front" depends on the order of the corners, so we check the order
    // against `outward` (the way the face should point) and flip it if needed.
    void quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 outward, Color color) {
        const Vector3 normal = cross(subtract(b, a), subtract(c, a));
        if (dot(normal, outward) < 0.0f) {
            std::swap(b, d); // reverse the winding
        }
        vertex(a, color); vertex(b, color); vertex(c, color);
        vertex(a, color); vertex(c, color); vertex(d, color);
    }

    int vertexCount() const { return static_cast<int>(positions.size() / 3); }
};

// Copies the collected triangles into a raylib Mesh and sends it to the GPU.
// raylib frees these arrays itself (UnloadModel), so they must come from
// raylib's allocator, not std::vector.
Mesh uploadMesh(const MeshBuilder& builder) {
    Mesh mesh{};
    mesh.vertexCount = builder.vertexCount();
    mesh.triangleCount = mesh.vertexCount / 3;

    const auto positionBytes = static_cast<unsigned int>(builder.positions.size() * sizeof(float));
    mesh.vertices = static_cast<float*>(MemAlloc(positionBytes));
    std::memcpy(mesh.vertices, builder.positions.data(), positionBytes);

    const auto colorBytes = static_cast<unsigned int>(builder.colors.size());
    mesh.colors = static_cast<unsigned char*>(MemAlloc(colorBytes));
    std::memcpy(mesh.colors, builder.colors.data(), colorBytes);

    UploadMesh(&mesh, false); // false = static: never updated after upload
    return mesh;
}

// The four neighbours of a tile: offset in x, offset in z.
constexpr std::array<std::array<int, 2>, 4> kNeighbours = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

} // namespace

WorldMesh::WorldMesh(const game::World& world) {
    MeshBuilder builder;
    const float groundBottom = -render::kGroundThickness;

    // Edges already added, so shared tile edges are drawn once. Key: the two
    // end points rounded to hundredths, smaller end first.
    std::set<std::array<int, 6>> seenEdges;
    auto addEdge = [&](Vector3 a, Vector3 b, Color color) {
        auto round = [](float v) { return static_cast<int>(v * 100.0f + (v < 0 ? -0.5f : 0.5f)); };
        std::array<int, 3> ka{round(a.x), round(a.y), round(a.z)};
        std::array<int, 3> kb{round(b.x), round(b.y), round(b.z)};
        if (kb < ka) {
            std::swap(ka, kb);
        }
        if (seenEdges.insert({ka[0], ka[1], ka[2], kb[0], kb[1], kb[2]}).second) {
            edges_.push_back({a, b, color});
        }
    };

    for (int z = 0; z < world.depth(); ++z) {
        for (int x = 0; x < world.width(); ++x) {
            const game::Tile tile = world.tileAt(x, z);
            const float top = surfaceHeight(tile);
            const float x0 = static_cast<float>(x), x1 = x0 + 1.0f;
            const float z0 = static_cast<float>(z), z1 = z0 + 1.0f;

            // Top face.
            builder.quad({x0, top, z0}, {x0, top, z1}, {x1, top, z1}, {x1, top, z0},
                         {0.0f, 1.0f, 0.0f}, topColor(tile));

            // Side faces, only where the neighbour's surface is lower.
            for (const auto& [dx, dz] : kNeighbours) {
                const int nx = x + dx, nz = z + dz;
                const bool inside = nx >= 0 && nz >= 0 && nx < world.width() && nz < world.depth();
                const float neighbourTop = inside ? surfaceHeight(world.tileAt(nx, nz)) : groundBottom;
                if (neighbourTop >= top) {
                    continue;
                }

                // The two ends of the edge shared with that neighbour, at the
                // neighbour's height; the face rises from there to our top.
                Vector3 a{}, b{};
                if (dx != 0) {
                    const float edgeX = dx > 0 ? x1 : x0;
                    a = {edgeX, neighbourTop, z0};
                    b = {edgeX, neighbourTop, z1};
                } else {
                    const float edgeZ = dz > 0 ? z1 : z0;
                    a = {x0, neighbourTop, edgeZ};
                    b = {x1, neighbourTop, edgeZ};
                }
                const Vector3 outward{static_cast<float>(dx), 0.0f, static_cast<float>(dz)};
                builder.quad(a, b, {b.x, top, b.z}, {a.x, top, a.z}, outward, sideColor(tile));
            }

            // Outlines: the top square of every tile, plus the vertical
            // corner lines of walls (the old cubes drew these too).
            const Color edge = edgeColor(tile);
            addEdge({x0, top, z0}, {x1, top, z0}, edge);
            addEdge({x1, top, z0}, {x1, top, z1}, edge);
            addEdge({x1, top, z1}, {x0, top, z1}, edge);
            addEdge({x0, top, z1}, {x0, top, z0}, edge);
            if (tile == game::Tile::Wall) {
                for (const auto& [cx, cz] : {std::array<float, 2>{x0, z0}, {x1, z0}, {x1, z1}, {x0, z1}}) {
                    addEdge({cx, 0.0f, cz}, {cx, top, cz}, edge);
                }
            }
        }
    }

    model_ = LoadModelFromMesh(uploadMesh(builder));
}

WorldMesh::~WorldMesh() { UnloadModel(model_); }

void WorldMesh::draw() const {
    // The whole static world in one call. WHITE = use the vertex colors as-is.
    DrawModel(model_, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    // Outlines as one batch of lines.
    rlBegin(RL_LINES);
    for (const EdgeLine& line : edges_) {
        rlColor4ub(line.color.r, line.color.g, line.color.b, line.color.a);
        rlVertex3f(line.from.x, line.from.y, line.from.z);
        rlVertex3f(line.to.x, line.to.y, line.to.z);
    }
    rlEnd();
}

} // namespace platform
