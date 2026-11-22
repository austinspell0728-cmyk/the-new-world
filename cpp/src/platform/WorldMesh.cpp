// WorldMesh.cpp
//
// How the mesh is built, tile by tile:
//   - every tile gets a flat top face at its surface height (ground 0,
//     water slightly below, walls and buildings at their height);
//   - a side face is added only where a neighbouring surface is LOWER, so
//     the face between two wall tiles, or between two ground tiles, is never
//     generated;
//   - props (trees, stalls, the well) stand on a ground tile and are built
//     from a few small boxes;
//   - colors are stored per vertex, so one mesh holds every tile type with
//     no textures. Vertical faces are shaded slightly darker by direction
//     (see RenderConfig) because there is no lighting.
// Thin outlines along tile, wall, building and prop edges are collected
// once and drawn as one batch of lines.

#include "WorldMesh.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>

#include "rlgl.h"

#include "RenderConfig.h"

namespace platform {

namespace {

using game::Tile;

// --- Small vector helpers ------------------------------------------------------

Vector3 subtract(Vector3 a, Vector3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

Vector3 cross(Vector3 a, Vector3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float dot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Color shade(Color c, float factor) {
    return {static_cast<unsigned char>(c.r * factor), static_cast<unsigned char>(c.g * factor),
            static_cast<unsigned char>(c.b * factor), c.a};
}

// --- Collecting triangles and outlines -------------------------------------------

class MeshBuilder {
public:
    // Adds a flat four-cornered face (corners listed in order around its
    // edge) as two triangles. The GPU skips triangles seen from behind, and
    // "front" depends on the order of the corners, so we check the order
    // against `outward` (the way the face should point) and flip it if
    // needed. Vertical faces get the directional shading.
    void quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 outward, Color color) {
        const Vector3 normal = cross(subtract(b, a), subtract(c, a));
        if (dot(normal, outward) < 0.0f) {
            std::swap(b, d); // reverse the winding
        }
        if (outward.x != 0.0f) {
            color = shade(color, render::kShadeFacingX);
        } else if (outward.z != 0.0f) {
            color = shade(color, render::kShadeFacingZ);
        }
        vertex(a, color); vertex(b, color); vertex(c, color);
        vertex(a, color); vertex(c, color); vertex(d, color);
    }

    // A solid box from corner `lo` to corner `hi` (all six faces), with its
    // own top color, plus its 12 outline edges.
    void box(Vector3 lo, Vector3 hi, Color side, Color top) {
        quad({lo.x, hi.y, lo.z}, {lo.x, hi.y, hi.z}, {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}, {0, 1, 0}, top);
        quad({lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {lo.x, lo.y, hi.z}, {0, -1, 0}, side);
        quad({hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}, {1, 0, 0}, side);
        quad({lo.x, lo.y, lo.z}, {lo.x, lo.y, hi.z}, {lo.x, hi.y, hi.z}, {lo.x, hi.y, lo.z}, {-1, 0, 0}, side);
        quad({lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}, {0, 0, 1}, side);
        quad({lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z}, {0, 0, -1}, side);

        for (const float y : {lo.y, hi.y}) {
            edge({lo.x, y, lo.z}, {hi.x, y, lo.z}, render::kPropEdgeColor);
            edge({hi.x, y, lo.z}, {hi.x, y, hi.z}, render::kPropEdgeColor);
            edge({hi.x, y, hi.z}, {lo.x, y, hi.z}, render::kPropEdgeColor);
            edge({lo.x, y, hi.z}, {lo.x, y, lo.z}, render::kPropEdgeColor);
        }
        for (const auto& [x, z] : {std::array<float, 2>{lo.x, lo.z}, {hi.x, lo.z}, {hi.x, hi.z}, {lo.x, hi.z}}) {
            edge({x, lo.y, z}, {x, hi.y, z}, render::kPropEdgeColor);
        }
    }

    // Adds an outline segment, skipping duplicates (neighbouring tiles share
    // edges). The first color added for an edge wins.
    void edge(Vector3 a, Vector3 b, Color color) {
        auto key = [](Vector3 p) {
            auto round = [](float v) { return static_cast<int>(v * 100.0f + (v < 0 ? -0.5f : 0.5f)); };
            return std::array<int, 3>{round(p.x), round(p.y), round(p.z)};
        };
        std::array<int, 3> ka = key(a), kb = key(b);
        if (kb < ka) {
            std::swap(ka, kb);
        }
        if (seenEdges_.insert({ka[0], ka[1], ka[2], kb[0], kb[1], kb[2]}).second) {
            edges.push_back({a, b, color});
        }
    }

    // Copies the collected triangles into a raylib Mesh and sends it to the
    // GPU. raylib frees these arrays itself (UnloadModel), so they must come
    // from raylib's allocator, not std::vector.
    Mesh upload() const {
        Mesh mesh{};
        mesh.vertexCount = static_cast<int>(positions_.size() / 3);
        mesh.triangleCount = mesh.vertexCount / 3;

        const auto positionBytes = static_cast<unsigned int>(positions_.size() * sizeof(float));
        mesh.vertices = static_cast<float*>(MemAlloc(positionBytes));
        std::memcpy(mesh.vertices, positions_.data(), positionBytes);

        const auto colorBytes = static_cast<unsigned int>(colors_.size());
        mesh.colors = static_cast<unsigned char*>(MemAlloc(colorBytes));
        std::memcpy(mesh.colors, colors_.data(), colorBytes);

        UploadMesh(&mesh, false); // false = static: never updated after upload
        return mesh;
    }

    struct Edge {
        Vector3 from;
        Vector3 to;
        Color color;
    };
    std::vector<Edge> edges;

private:
    void vertex(Vector3 p, Color c) {
        positions_.insert(positions_.end(), {p.x, p.y, p.z});
        colors_.insert(colors_.end(), {c.r, c.g, c.b, c.a});
    }

    std::vector<float> positions_;      // x, y, z per vertex
    std::vector<unsigned char> colors_; // r, g, b, a per vertex
    std::set<std::array<int, 6>> seenEdges_;
};

// --- Look of each tile type ----------------------------------------------------

// Height of a tile's top surface. Props count as ground: they stand on it.
float surfaceHeight(Tile tile) {
    switch (tile) {
    case Tile::Water: return -render::kWaterDepth;
    case Tile::Wall: return render::kWallHeight;
    case Tile::Building: return render::kBuildingHeight;
    case Tile::Grass:
    case Tile::Path:
    case Tile::Plaza:
    case Tile::Tree:
    case Tile::Stall:
    case Tile::Well: return 0.0f;
    }
    return 0.0f;
}

// Color of the ground under a tile (or the top of a wall). Buildings are
// colored per building, see buildingTop/Side below.
Color groundColor(Tile tile) {
    switch (tile) {
    case Tile::Grass:
    case Tile::Tree: return render::kGrassColor;
    case Tile::Path: return render::kPathColor;
    case Tile::Plaza:
    case Tile::Stall:
    case Tile::Well: return render::kPlazaColor;
    case Tile::Water: return render::kWaterColor;
    case Tile::Wall: return render::kWallTopColor;
    case Tile::Building: return MAGENTA; // never used: buildings have their own colors
    }
    return MAGENTA; // Bright "missing" color makes a forgotten tile type obvious.
}

// --- Props, each standing in the middle of its tile ------------------------------

void addTree(MeshBuilder& mesh, float cx, float cz) {
    mesh.box({cx - 0.12f, 0.0f, cz - 0.12f}, {cx + 0.12f, 1.0f, cz + 0.12f}, render::kTrunkColor,
             render::kTrunkColor);
    mesh.box({cx - 0.45f, 0.9f, cz - 0.45f}, {cx + 0.45f, 1.9f, cz + 0.45f}, render::kLeafColor,
             render::kLeafTopColor);
    mesh.box({cx - 0.28f, 1.9f, cz - 0.28f}, {cx + 0.28f, 2.4f, cz + 0.28f}, render::kLeafColor,
             render::kLeafTopColor);
}

void addStall(MeshBuilder& mesh, float x0, float z0, Color awning) {
    const float x1 = x0 + 1.0f, z1 = z0 + 1.0f;
    // Counter.
    mesh.box({x0 + 0.08f, 0.0f, z0 + 0.15f}, {x1 - 0.08f, 0.9f, z1 - 0.15f}, render::kStallWoodColor,
             render::kStallWoodColor);
    // Four corner posts holding up the awning.
    for (const auto& [px, pz] : {std::array<float, 2>{x0 + 0.08f, z0 + 0.08f}, {x1 - 0.16f, z0 + 0.08f},
                                 {x0 + 0.08f, z1 - 0.16f}, {x1 - 0.16f, z1 - 0.16f}}) {
        mesh.box({px, 0.9f, pz}, {px + 0.08f, 1.75f, pz + 0.08f}, render::kStallWoodColor,
                 render::kStallWoodColor);
    }
    // Awning.
    mesh.box({x0 + 0.02f, 1.75f, z0 + 0.02f}, {x1 - 0.02f, 1.85f, z1 - 0.02f}, awning, awning);
}

void addWell(MeshBuilder& mesh, float cx, float cz) {
    // Stone base with dark water showing on top.
    mesh.box({cx - 0.35f, 0.0f, cz - 0.35f}, {cx + 0.35f, 0.7f, cz + 0.35f}, render::kStoneColor,
             render::kWellWaterColor);
    // Two posts and a small roof.
    for (const float px : {cx - 0.34f, cx + 0.26f}) {
        mesh.box({px, 0.7f, cz - 0.04f}, {px + 0.08f, 1.6f, cz + 0.04f}, render::kStallWoodColor,
                 render::kStallWoodColor);
    }
    mesh.box({cx - 0.45f, 1.6f, cz - 0.3f}, {cx + 0.45f, 1.72f, cz + 0.3f}, render::kTrunkColor,
             render::kTrunkColor);
}

// The four neighbours of a tile: offset in x, offset in z.
constexpr std::array<std::array<int, 2>, 4> kNeighbours = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

} // namespace

WorldMesh::WorldMesh(const game::World& world, const BuildingMap& buildings,
                     const std::vector<bool>& modelled, const std::vector<bool>& modelledTrees) {
    MeshBuilder mesh;
    const int width = world.width(), depth = world.depth();
    const float groundBottom = -render::kGroundThickness;

    auto inside = [&](int x, int z) { return x >= 0 && z >= 0 && x < width && z < depth; };
    // A building drawn from kit models (BuildingModels) is left out of this
    // mesh: its tiles are treated as plain path, which the house stands on.
    auto isModelled = [&](int id) {
        return id >= 0 && static_cast<std::size_t>(id) < modelled.size() && modelled[static_cast<std::size_t>(id)];
    };
    auto buildingAt = [&](int x, int z) {
        const int id = buildings.idAt(x, z);
        return isModelled(id) ? -1 : id;
    };
    auto tileAt = [&](int x, int z) {
        return isModelled(buildings.idAt(x, z)) ? Tile::Path : world.tileAt(x, z);
    };

    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            const Tile tile = tileAt(x, z);
            const float top = surfaceHeight(tile);
            const float x0 = static_cast<float>(x), x1 = x0 + 1.0f;
            const float z0 = static_cast<float>(z), z1 = z0 + 1.0f;
            const int building = buildingAt(x, z);

            // Colors for this tile: buildings use their building's colors.
            Color topColor = groundColor(tile);
            Color sideColor = (tile == Tile::Wall) ? render::kWallColor : topColor;
            Color outline = (tile == Tile::Wall) ? render::kWallEdgeColor : render::kTileEdgeColor;
            if (building >= 0) {
                const auto b = static_cast<std::size_t>(building);
                sideColor = render::kBuildingWallColors[b % render::kBuildingWallColors.size()];
                topColor = render::kBuildingRoofColors[b % render::kBuildingRoofColors.size()];
                outline = render::kBuildingEdgeColor;
            }

            // Top face.
            mesh.quad({x0, top, z0}, {x0, top, z1}, {x1, top, z1}, {x1, top, z0}, {0, 1, 0}, topColor);

            // Side faces, only where the neighbour's surface is lower.
            for (const auto& [dx, dz] : kNeighbours) {
                const int nx = x + dx, nz = z + dz;
                const Tile neighbour = inside(nx, nz) ? tileAt(nx, nz) : Tile::Wall;
                const float neighbourTop = inside(nx, nz) ? surfaceHeight(neighbour) : groundBottom;
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
                mesh.quad(a, b, {b.x, top, b.z}, {a.x, top, a.z}, outward, sideColor);

                // A door wherever a path or the plaza meets a building side,
                // drawn just in front of the wall so it never flickers into it.
                if (building >= 0 && (neighbour == Tile::Path || neighbour == Tile::Plaza)) {
                    const float gap = 0.02f;
                    const float half = render::kDoorWidth / 2.0f;
                    const Vector3 mid{(a.x + b.x) / 2.0f + dx * gap, 0.0f, (a.z + b.z) / 2.0f + dz * gap};
                    const Vector3 along{static_cast<float>(dz != 0), 0.0f, static_cast<float>(dx != 0)};
                    const Vector3 left{mid.x - along.x * half, 0.0f, mid.z - along.z * half};
                    const Vector3 right{mid.x + along.x * half, 0.0f, mid.z + along.z * half};
                    mesh.quad(left, right, {right.x, render::kDoorHeight, right.z},
                              {left.x, render::kDoorHeight, left.z}, outward, render::kDoorColor);
                }
            }

            // Outlines along this tile's top edges. Walls outline every tile
            // (they read as stone blocks); buildings only where the building
            // ends, so each building reads as one shape.
            const std::array<std::array<Vector3, 2>, 4> topEdges = {{
                {Vector3{x1, top, z0}, Vector3{x1, top, z1}}, // +X side
                {Vector3{x0, top, z0}, Vector3{x0, top, z1}}, // -X side
                {Vector3{x0, top, z1}, Vector3{x1, top, z1}}, // +Z side
                {Vector3{x0, top, z0}, Vector3{x1, top, z0}}, // -Z side
            }};
            for (std::size_t i = 0; i < kNeighbours.size(); ++i) {
                const bool sameBuilding =
                    building >= 0 && buildingAt(x + kNeighbours[i][0], z + kNeighbours[i][1]) == building;
                if (!sameBuilding) {
                    mesh.edge(topEdges[i][0], topEdges[i][1], outline);
                }
            }

            // Vertical outlines: every corner of a wall tile; only the outer
            // and inner corners of a building.
            for (const auto& [cx, cz] : {std::array<int, 2>{x, z}, {x + 1, z}, {x + 1, z + 1}, {x, z + 1}}) {
                bool draw = (tile == Tile::Wall);
                if (building >= 0) {
                    // Count this building's tiles among the four around the corner.
                    int count = 0;
                    for (const auto& [ox, oz] : {std::array<int, 2>{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}) {
                        count += (buildingAt(cx + ox, cz + oz) == building) ? 1 : 0;
                    }
                    draw = (count == 1 || count == 3);
                }
                if (draw) {
                    mesh.edge({static_cast<float>(cx), 0.0f, static_cast<float>(cz)},
                              {static_cast<float>(cx), top, static_cast<float>(cz)}, outline);
                }
            }

            // Props stand on top of their ground tile.
            const float cx = x0 + 0.5f, cz = z0 + 0.5f;
            switch (tile) {
            case Tile::Tree: {
                // Model trees (TreeModels) replace the box tree; the grass
                // under them stays.
                const auto index = static_cast<std::size_t>(z * width + x);
                if (index >= modelledTrees.size() || !modelledTrees[index]) {
                    addTree(mesh, cx, cz);
                }
                break;
            }
            case Tile::Stall:
                addStall(mesh, x0, z0, render::kAwningColors[static_cast<std::size_t>(x + z) %
                                                             render::kAwningColors.size()]);
                break;
            case Tile::Well: addWell(mesh, cx, cz); break;
            default: break;
            }
        }
    }

    model_ = LoadModelFromMesh(mesh.upload());
    for (const MeshBuilder::Edge& e : mesh.edges) {
        edges_.push_back({e.from, e.to, e.color});
    }
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
