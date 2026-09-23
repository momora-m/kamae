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

#include <string>

constexpr float kCameraPitchLimit = 1.48f;
constexpr float kCameraDistanceMin = 1.5f;
constexpr float kCameraDistanceMax = 20.0f;

struct SceneState {
    float clear_color[3] = {0.09f, 0.10f, 0.12f};
    float cube_position[3] = {0.0f, 0.0f, 0.0f};
    float cube_rotation_degrees[3] = {0.0f, 0.0f, 0.0f};
    float camera_distance = 3.5f;
    float camera_yaw = 0.65f;
    float camera_pitch = 0.40f;
};

// Win32 window, DirectX 11 device, one lit cube, and a ground plate in an offscreen target.
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
