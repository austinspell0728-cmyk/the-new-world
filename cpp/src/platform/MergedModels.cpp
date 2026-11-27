// MergedModels.cpp

#include "MergedModels.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <set>
#include <type_traits>
#include <utility>

#include "RaylibMath.h"
#include "rlgl.h"

#include "RenderConfig.h"

namespace platform {

namespace {

// Meshes use 16-bit vertex indices, so one mesh holds at most this many
// vertices. A material with more geometry is split into several meshes.
constexpr int kMaxVerticesPerMesh = 65535;

// Pixels whose texture alpha is below this are not drawn (alphaCutout).
// Replaces raylib's default fragment shader, keeping its inputs: the
// texture, the material color and the vertex color (our fake light).
constexpr const char* kCutoutFragmentShader = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    if (texel.a < 0.5) discard;
    finalColor = vec4(texel.rgb, 1.0) * colDiffuse * fragColor;
}
)";

// The names of a glTF file's materials, in file order. raylib does not keep
// them, but merging by name lets the same material from different files
// (e.g. wood trim on walls, corners and roofs) share one mesh.
// The glTF "materials" array is scanned for each material's "name"; returns
// an empty list if it cannot be read, and the caller then keeps each file's
// materials separate.
std::vector<std::string> gltfMaterialNames(const std::string& path) {
    std::vector<std::string> names;
    char* raw = LoadFileText(path.c_str());
    if (raw == nullptr) {
        return names;
    }
    const std::string text(raw);
    UnloadFileText(raw);

    std::size_t pos = text.find("\"materials\"");
    pos = (pos == std::string::npos) ? pos : text.find('[', pos);
    if (pos == std::string::npos) {
        return names;
    }
    // Walk the array, skipping strings, and pick up "name" keys that sit
    // directly in a material object (depth 2: array, then object).
    int depth = 0;
    for (std::size_t i = pos; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '[' || c == '{') {
            ++depth;
        } else if (c == ']' || c == '}') {
            if (--depth == 0) {
                break; // end of the materials array
            }
        } else if (c == '"') {
            const std::size_t end = text.find('"', i + 1);
            if (end == std::string::npos) {
                return {};
            }
            const std::string word = text.substr(i + 1, end - i - 1);
            i = end;
            if (depth == 2 && word == "name") {
                const std::size_t open = text.find('"', text.find(':', end));
                const std::size_t close = text.find('"', open + 1);
                if (open == std::string::npos || close == std::string::npos) {
                    return {};
                }
                names.push_back(text.substr(open + 1, close - open - 1));
                i = close;
            }
        }
    }
    return names;
}

// Fake light, as for the box world: brightness from which way a surface
// faces (we draw without lights, see RenderConfig). Faces pointing up or
// down keep full brightness; X-facing and Z-facing faces get the same
// darkening as the box walls, blended for anything in between.
unsigned char shadedChannel(float normalX, float normalY, float normalZ) {
    const float brightness = normalY * normalY + normalX * normalX * render::kShadeFacingX +
                             normalZ * normalZ * render::kShadeFacingZ;
    return static_cast<unsigned char>(std::lround(255.0f * std::clamp(brightness, 0.0f, 1.0f)));
}

// Turns a normal by the 3x3 part of `m` (no translation) and normalises it.
Vector3 transformNormal(Vector3 n, const Matrix& m) {
    return Vector3Normalize({m.m0 * n.x + m.m4 * n.y + m.m8 * n.z, m.m1 * n.x + m.m5 * n.y + m.m9 * n.z,
                             m.m2 * n.x + m.m6 * n.y + m.m10 * n.z});
}

} // namespace

MergedModels::MergedModels(std::string assetDir, std::string label, bool alphaCutout)
    : assetDir_(std::move(assetDir)), label_(std::move(label)), alphaCutout_(alphaCutout) {}

MergedModels::~MergedModels() {
    for (Batch& batch : batches_) {
        UnloadMesh(batch.mesh); // the material belongs to a source model
    }
    for (Model& model : sources_) {
        UnloadModel(model);
    }
    if (cutoutShader_.id != 0 && cutoutShader_.id != rlGetShaderIdDefault()) {
        UnloadShader(cutoutShader_);
    }
}

bool MergedModels::load(const std::string& file) {
    auto found = loaded_.find(file);
    if (found != loaded_.end()) {
        return found->second.model >= 0;
    }
    Source source;
    const std::string path = assetDir_ + file;
    if (!FileExists(path.c_str())) {
        std::cerr << label_ << ": missing model file '" << path << "'.\n";
    } else {
        Model model = LoadModel(path.c_str());
        if (model.meshCount == 0 || !IsModelValid(model)) {
            std::cerr << label_ << ": could not load model '" << path << "'.\n";
            UnloadModel(model);
        } else {
            // Mipmaps stop the textures shimmering when seen from afar.
            std::set<unsigned int> done;
            for (int m = 0; m < model.materialCount; ++m) {
                Texture2D& texture = model.materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
                if (texture.id != rlGetTextureIdDefault() && done.insert(texture.id).second) {
                    GenTextureMipmaps(&texture);
                    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
                }
            }
            // raylib puts a default material at index 0, then the file's
            // materials in order.
            const std::vector<std::string> names = gltfMaterialNames(path);
            const bool namesFit = static_cast<int>(names.size()) + 1 == model.materialCount;
            for (int m = 0; m < model.materialCount; ++m) {
                source.keys.push_back((namesFit && m > 0) ? names[static_cast<std::size_t>(m - 1)]
                                                          : file + "#" + std::to_string(m));
            }
            source.model = static_cast<int>(sources_.size());
            sources_.push_back(model);
        }
    }
    const bool ok = source.model >= 0;
    loaded_.emplace(file, std::move(source));
    return ok;
}

void MergedModels::add(const std::string& file, const Matrix& toWorld) {
    const Source& source = loaded_.at(file);
    const Model& model = sources_[static_cast<std::size_t>(source.model)];
    // Normals need the inverse-transpose so stretched copies still shade
    // correctly.
    const Matrix normalMatrix = MatrixTranspose(MatrixInvert(toWorld));

    for (int i = 0; i < model.meshCount; ++i) {
        const Mesh& mesh = model.meshes[i];
        const int materialIndex = model.meshMaterial[i];
        Group& group = groups_[source.keys[static_cast<std::size_t>(materialIndex)]];
        if (group.meshes.empty()) {
            group.material = model.materials[materialIndex];
        }
        if (group.meshes.empty() || group.meshes.back().vertexCount() + mesh.vertexCount > kMaxVerticesPerMesh) {
            group.meshes.emplace_back();
        }
        MeshData& out = group.meshes.back();
        const auto base = static_cast<unsigned short>(out.vertexCount());

        for (int v = 0; v < mesh.vertexCount; ++v) {
            const Vector3 local{mesh.vertices[v * 3], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]};
            const Vector3 p = Vector3Transform(local, toWorld);
            out.positions.insert(out.positions.end(), {p.x, p.y, p.z});
            out.texcoords.insert(out.texcoords.end(), {mesh.texcoords ? mesh.texcoords[v * 2] : 0.0f,
                                                       mesh.texcoords ? mesh.texcoords[v * 2 + 1] : 0.0f});
            unsigned char shade = 255;
            if (mesh.normals != nullptr) {
                const Vector3 n = transformNormal(
                    {mesh.normals[v * 3], mesh.normals[v * 3 + 1], mesh.normals[v * 3 + 2]}, normalMatrix);
                shade = shadedChannel(n.x, n.y, n.z);
            }
            out.colors.insert(out.colors.end(), {shade, shade, shade, 255});
        }
        if (mesh.indices != nullptr) {
            for (int t = 0; t < mesh.triangleCount * 3; ++t) {
                out.indices.push_back(static_cast<unsigned short>(base + mesh.indices[t]));
            }
        } else {
            for (int t = 0; t < mesh.vertexCount; ++t) {
                out.indices.push_back(static_cast<unsigned short>(base + t));
            }
        }
    }
}

void MergedModels::finish() {
    if (alphaCutout_ && !groups_.empty()) {
        cutoutShader_ = LoadShaderFromMemory(nullptr, kCutoutFragmentShader);
    }
    for (auto& [key, group] : groups_) {
        Material material = group.material; // shares the source model's texture
        if (cutoutShader_.id != 0) {
            material.shader = cutoutShader_;
        }
        for (const MeshData& data : group.meshes) {
            // Copy into raylib-owned arrays (UnloadMesh frees them) and upload.
            Mesh mesh{};
            mesh.vertexCount = data.vertexCount();
            mesh.triangleCount = static_cast<int>(data.indices.size() / 3);
            auto copy = [](const auto& source) {
                using T = typename std::decay_t<decltype(source)>::value_type;
                const auto bytes = static_cast<unsigned int>(source.size() * sizeof(T));
                T* target = static_cast<T*>(MemAlloc(bytes));
                std::memcpy(target, source.data(), bytes);
                return target;
            };
            mesh.vertices = copy(data.positions);
            mesh.texcoords = copy(data.texcoords);
            mesh.colors = copy(data.colors);
            mesh.indices = copy(data.indices);
            UploadMesh(&mesh, false); // static: never changes after upload
            batches_.push_back({mesh, material});
        }
    }
    groups_.clear(); // the CPU copies are no longer needed
}

void MergedModels::draw() const {
    // Kit materials are double-sided (thin trims, leaf cards, the inside of
    // walls seen through windows), so don't skip back faces here.
    rlDisableBackfaceCulling();
    for (const Batch& batch : batches_) {
        DrawMesh(batch.mesh, batch.material, MatrixIdentity());
    }
    rlEnableBackfaceCulling();
}

} // namespace platform
