#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "imgui.h"

#include "attack_mark.hpp"

#include <string>

constexpr float kCameraPitchLimit = 1.48f;
constexpr float kCameraDistanceMin = 1.5f;
constexpr float kCameraDistanceMax = 20.0f;
constexpr float kCubeHalfExtent = 0.5f;
constexpr float kFloorHalfExtent = 20.0f;
constexpr float kFloorHalfMin = 4.0f;
constexpr float kFloorHalfMax = 40.0f;
constexpr int kSubjectCapacity = 4;
constexpr int kPlayer = 0;

// Editing keeps time stopped. A trial starts only from an explicit Start.
enum class SessionMode {
    Editing,
    Trial,
};

// One body in the shared list. Index 0 is the player. Further bodies are more elements.
// subject_count is how many slots are in use. The array length is the maximum.
// remaining starts at 3. Zero is not drawn, not a walk obstacle, and not an attack target.
struct Subject {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation_degrees[3] = {0.0f, 0.0f, 0.0f};
    float color[3] = {0.78f, 0.48f, 0.27f};
    int remaining = 3;
    float attack_cooldown = 0.0f;
};

struct SceneState {
    float clear_color[3] = {0.09f, 0.10f, 0.12f};
    // Builtin 1v1, separate from saved scenes. The player is at the origin, yaw 0.
    // One opponent is at (0, 0, 4), yaw 180, facing -Z. The floor half is 20.
    // At this distance the attack box does not reach. Unused slots stay past
    // subject_count and are not simulated.
    int subject_count = 2;
    float floor_half = kFloorHalfExtent;
    SessionMode session = SessionMode::Editing;
    std::string layout_error;
    Subject subjects[kSubjectCapacity] = {
        Subject{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.78f, 0.48f, 0.27f}, 3},
        Subject{{0.0f, 0.0f, 4.0f}, {0.0f, 180.0f, 0.0f}, {0.25f, 0.42f, 0.68f}, 3},
    };
    float camera_distance = 3.5f;
    float camera_yaw = 0.65f;
    float camera_pitch = 0.40f;
    bool walk_with_camera_yaw = false;
    AttackMark attack_marks[kSubjectCapacity]{};
};

static_assert(kPlayer == 0, "the player is the first subject");
static_assert(kSubjectCapacity == 4, "the list holds the player and three more");
static_assert(kSubjectCapacity > kPlayer, "the player fits in the list");

// Win32 window, DirectX 11 device, one lit cube per subject, and a ground plate in an offscreen target.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    bool Initialize(HWND hwnd, std::wstring& error);
    void RequestResize(UINT width, UINT height);
    bool PrepareFrame();
    void DrawScene(const SceneState& scene, UINT width, UINT height);
    void BindAndClearBackBuffer();
    void Present();
    void Shutdown();

    bool DeviceLost() const { return device_lost_; }
    ID3D11Device* Device() const { return device_.Get(); }
    ID3D11DeviceContext* Context() const { return context_.Get(); }
    ImTextureID SceneColorTexture() const;
    const std::string& ShaderError() const { return shader_error_; }

private:
    bool CreateBackBuffer(std::wstring& error);
    void ReleaseBackBuffer();
    bool CreatePipeline(std::wstring& error);
    bool CompileShaders(std::string& error);
    bool EnsureSceneTarget(UINT width, UINT height);
    void UnbindTargets();

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backbuffer_rtv_;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> scene_color_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_rtv_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_srv_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> scene_depth_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> scene_dsv_;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> floor_vertex_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> floor_index_buffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_state_;

    UINT scene_width_ = 0;
    UINT scene_height_ = 0;
    UINT pending_width_ = 0;
    UINT pending_height_ = 0;
    UINT index_count_ = 0;
    UINT floor_index_count_ = 0;
    bool swap_chain_occluded_ = false;
    bool device_lost_ = false;
    std::string shader_error_;
};
