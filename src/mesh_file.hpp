#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// Visual bounds after glTF axis conversion. The collision cube stays ±0.5.
// X and Z may pass the cube by 0.15. Y may pass it by 0.10 above, and not below.
constexpr float kCharacterMeshMinX = -0.65f;
constexpr float kCharacterMeshMaxX = 0.65f;
constexpr float kCharacterMeshMinY = -0.5f;
constexpr float kCharacterMeshMaxY = 0.6f;
constexpr float kCharacterMeshMinZ = -0.65f;
constexpr float kCharacterMeshMaxZ = 0.65f;
constexpr int kCharacterMeshMaxVertices = 65535;

// One triangle mesh in engine space. +Y is up, +Z is forward, +X is right.
// positions and normals are tightly packed xyz floats. indices are triangles.
struct LoadedMesh {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<std::uint16_t> indices;
};

// Reads a glTF 2.0 .gltf plus a sibling .bin. Negates X so glTF's right (-X)
// becomes this engine's right (+X). On failure, mesh is cleared and error is set.
bool LoadCharacterMesh(const std::filesystem::path& gltf_path, LoadedMesh& mesh, std::string& error);
