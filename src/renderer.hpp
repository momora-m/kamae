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
constexpr int kSubjectCount = 2;
constexpr int kPlayer = 0;

// One body in the shared list. Index 0 is the player. A third body is another element.
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
    // Player stays at the origin. Opponent is (0, 0, 4), yaw 180, facing the player along -Z.
    Subject subjects[kSubjectCount] = {
        Subject{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.78f, 0.48f, 0.27f}, 3},
        Subject{{0.0f, 0.0f, 4.0f}, {0.0f, 180.0f, 0.0f}, {0.25f, 0.42f, 0.68f}, 3},
    };
    float camera_distance = 3.5f;
    float camera_yaw = 0.65f;
    float camera_pitch = 0.40f;
    bool walk_with_camera_yaw = false;
    AttackMark attack_mark{};
};

static_assert(kPlayer >= 0 && kPlayer < kSubjectCount, "the player is the first subject");

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
