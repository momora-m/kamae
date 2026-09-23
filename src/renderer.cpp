#include "renderer.hpp"

#include <DirectXMath.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

namespace {

struct Vertex {
    float position[3];
    float normal[3];
};

// Matches shaders/lit.hlsl register(b0). row-major XMFLOAT4X4, 16-byte aligned.
struct FrameConstants {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view_projection;
    DirectX::XMFLOAT4 light_direction;
    DirectX::XMFLOAT4 light_color;
    DirectX::XMFLOAT4 ambient_color;
    DirectX::XMFLOAT4 camera_position;
    DirectX::XMFLOAT4 albedo;
};

static_assert(sizeof(FrameConstants) % 16 == 0, "constant buffer size must be a multiple of 16");

struct Face {
    float normal[3];
    float corners[4][3];
};

// Clockwise when seen from outside. Direct3D treats clockwise as front-facing
// when FrontCounterClockwise is false, so back-face culling keeps the exterior.
constexpr Face kFaces[] = {
    {{0.0f, 0.0f, 1.0f}, {{0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}}},
    {{0.0f, 0.0f, -1.0f}, {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}}},
    {{0.0f, 1.0f, 0.0f}, {{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}}},
    {{0.0f, -1.0f, 0.0f}, {{0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}}},
    {{1.0f, 0.0f, 0.0f}, {{0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}}},
    {{-1.0f, 0.0f, 0.0f}, {{-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}}},
};

std::wstring HresultMessage(const wchar_t* context, HRESULT hr) {
    wchar_t text[256]{};
    swprintf_s(text, L"%s\nHRESULT 0x%08X", context, static_cast<unsigned>(hr));
    return text;
}

std::filesystem::path ExecutableDirectory() {
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::string ReadTextFile(const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Could not open lit.hlsl next to editor.exe.";
        return {};
    }
    std::ostringstream stream;
    stream << input.rdbuf();
    if (!input && !input.eof()) {
        error = "Could not read lit.hlsl.";
        return {};
    }
    return stream.str();
}

void DrawLitMesh(
    ID3D11DeviceContext* context,
    ID3D11Buffer* constant_buffer,
    ID3D11Buffer* vertex_buffer,
    ID3D11Buffer* index_buffer,
    UINT index_count,
    const FrameConstants& constants) {
    if (context == nullptr || constant_buffer == nullptr || vertex_buffer == nullptr || index_buffer == nullptr ||
        index_count == 0) {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return;
    }
    std::memcpy(mapped.pData, &constants, sizeof(constants));
    context->Unmap(constant_buffer, 0);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
    context->VSSetConstantBuffers(0, 1, &constant_buffer);
    context->PSSetConstantBuffers(0, 1, &constant_buffer);
    context->DrawIndexed(index_count, 0, 0);
}

std::string BlobMessage(ID3DBlob* blob) {
    if (blob == nullptr || blob->GetBufferSize() == 0) {
        return {};
    }
    const char* text = static_cast<const char*>(blob->GetBufferPointer());
    std::size_t size = blob->GetBufferSize();
    while (size > 0 && text[size - 1] == '\0') {
        --size;
    }
    return std::string(text, size);
}

}  // namespace

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(HWND hwnd, std::wstring& error) {
    DXGI_SWAP_CHAIN_DESC swap_desc{};
    swap_desc.BufferCount = 2;
    swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.OutputWindow = hwnd;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.Windowed = TRUE;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_levels,
        1,
        D3D11_SDK_VERSION,
        &swap_desc,
        swap_chain_.GetAddressOf(),
        device_.GetAddressOf(),
        nullptr,
        context_.GetAddressOf());
    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            feature_levels,
            1,
            D3D11_SDK_VERSION,
            &swap_desc,
            swap_chain_.GetAddressOf(),
            device_.GetAddressOf(),
            nullptr,
            context_.GetAddressOf());
    }
    if (FAILED(hr)) {
        error = HresultMessage(L"DirectX 11 デバイスを作成できませんでした。", hr);
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    if (SUCCEEDED(swap_chain_->GetParent(IID_PPV_ARGS(factory.GetAddressOf())))) {
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    }

    if (!CreateBackBuffer(error)) {
        return false;
    }
    if (!CreatePipeline(error)) {
        return false;
    }

    shader_error_.clear();
    if (!CompileShaders(shader_error_)) {
        // The window still opens so the shader message is visible in the Render panel.
        vertex_shader_.Reset();
        pixel_shader_.Reset();
        input_layout_.Reset();
    }
    return true;
}

void Renderer::RequestResize(UINT width, UINT height) {
    pending_width_ = width;
    pending_height_ = height;
}

bool Renderer::PrepareFrame() {
    if (device_lost_ || !swap_chain_) {
        return false;
    }

    if (swap_chain_occluded_) {
        const HRESULT hr = swap_chain_->Present(0, DXGI_PRESENT_TEST);
        if (hr == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            return false;
        }
        swap_chain_occluded_ = false;
    }

    if (pending_width_ != 0 && pending_height_ != 0) {
        ReleaseBackBuffer();
        const HRESULT hr = swap_chain_->ResizeBuffers(
            0, pending_width_, pending_height_, DXGI_FORMAT_UNKNOWN, 0);
        pending_width_ = 0;
        pending_height_ = 0;
        if (FAILED(hr)) {
            Sleep(10);
            return false;
        }
        std::wstring error;
        if (!CreateBackBuffer(error)) {
            Sleep(10);
            return false;
        }
    }

    return backbuffer_rtv_ != nullptr;
}

void Renderer::DrawScene(const SceneState& scene, UINT width, UINT height) {
    if (!context_ || width == 0 || height == 0) {
        return;
    }
    if (!EnsureSceneTarget(width, height)) {
        return;
    }

    ID3D11RenderTargetView* render_target = scene_rtv_.Get();
    context_->OMSetRenderTargets(1, &render_target, scene_dsv_.Get());

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    const float clear[4] = {
        scene.clear_color[0],
        scene.clear_color[1],
        scene.clear_color[2],
        1.0f,
    };
    context_->ClearRenderTargetView(scene_rtv_.Get(), clear);
    context_->ClearDepthStencilView(scene_dsv_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (!vertex_shader_ || !pixel_shader_ || !constant_buffer_) {
        UnbindTargets();
        return;
    }

    const float pitch = std::clamp(scene.camera_pitch, -1.48f, 1.48f);
    const float distance = std::clamp(scene.camera_distance, 1.5f, 20.0f);
    const float cos_pitch = std::cos(pitch);
    const DirectX::XMVECTOR target = DirectX::XMVectorSet(
        scene.cube_position[0], scene.cube_position[1], scene.cube_position[2], 0.0f);
    const DirectX::XMVECTOR eye = DirectX::XMVectorAdd(
        target,
        DirectX::XMVectorSet(
            distance * cos_pitch * std::sin(scene.camera_yaw),
            distance * std::sin(pitch),
            distance * cos_pitch * std::cos(scene.camera_yaw),
            0.0f));

    const DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
        eye, target, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(50.0f), aspect, 0.05f, 200.0f);
    FrameConstants constants{};
    DirectX::XMStoreFloat4x4(&constants.view_projection, view * projection);
    DirectX::XMStoreFloat4(
        &constants.light_direction,
        DirectX::XMVector3Normalize(DirectX::XMVectorSet(0.35f, 0.90f, 0.25f, 0.0f)));
    constants.light_color = {1.0f, 0.96f, 0.90f, 1.0f};
    constants.ambient_color = {0.14f, 0.15f, 0.18f, 1.0f};
    DirectX::XMStoreFloat4(&constants.camera_position, eye);

    context_->IASetInputLayout(input_layout_.Get());
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_shader_.Get(), nullptr, 0);
    context_->PSSetShader(pixel_shader_.Get(), nullptr, 0);
    context_->GSSetShader(nullptr, nullptr, 0);
    context_->RSSetState(rasterizer_.Get());
    context_->OMSetDepthStencilState(depth_state_.Get(), 0);
    context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    // Cube half-extent is 0.5 at y = 0, so the plate sits just under the bottom face.
    FrameConstants floor = constants;
    DirectX::XMStoreFloat4x4(&floor.world, DirectX::XMMatrixIdentity());
    floor.albedo = {0.32f, 0.34f, 0.33f, 1.0f};
    DrawLitMesh(
        context_.Get(),
        constant_buffer_.Get(),
        floor_vertex_buffer_.Get(),
        floor_index_buffer_.Get(),
        floor_index_count_,
        floor);

    const DirectX::XMMATRIX world =
        DirectX::XMMatrixRotationRollPitchYaw(
            DirectX::XMConvertToRadians(scene.cube_rotation_degrees[0]),
            DirectX::XMConvertToRadians(scene.cube_rotation_degrees[1]),
            DirectX::XMConvertToRadians(scene.cube_rotation_degrees[2])) *
        DirectX::XMMatrixTranslation(
            scene.cube_position[0], scene.cube_position[1], scene.cube_position[2]);
    DirectX::XMStoreFloat4x4(&constants.world, world);
    constants.albedo = {0.78f, 0.48f, 0.27f, 1.0f};
    DrawLitMesh(
        context_.Get(),
        constant_buffer_.Get(),
        vertex_buffer_.Get(),
        index_buffer_.Get(),
        index_count_,
        constants);

    UnbindTargets();
}

void Renderer::BindAndClearBackBuffer() {
    if (!context_ || !backbuffer_rtv_) {
        return;
    }
    ID3D11RenderTargetView* render_target = backbuffer_rtv_.Get();
    context_->OMSetRenderTargets(1, &render_target, nullptr);
    const float chrome[4] = {0.11f, 0.11f, 0.12f, 1.0f};
    context_->ClearRenderTargetView(render_target, chrome);
}

void Renderer::Present() {
    if (!swap_chain_) {
        return;
    }
    const HRESULT hr = swap_chain_->Present(1, 0);
    swap_chain_occluded_ = (hr == DXGI_STATUS_OCCLUDED);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        device_lost_ = true;
    }
}

void Renderer::Shutdown() {
    if (context_) {
        context_->ClearState();
        context_->Flush();
    }
    scene_dsv_.Reset();
    scene_depth_.Reset();
    scene_srv_.Reset();
    scene_rtv_.Reset();
    scene_color_.Reset();
    depth_state_.Reset();
    rasterizer_.Reset();
    constant_buffer_.Reset();
    floor_index_buffer_.Reset();
    floor_vertex_buffer_.Reset();
    index_buffer_.Reset();
    vertex_buffer_.Reset();
    input_layout_.Reset();
    pixel_shader_.Reset();
    vertex_shader_.Reset();
    backbuffer_rtv_.Reset();
    swap_chain_.Reset();
    context_.Reset();
    device_.Reset();
    scene_width_ = 0;
    scene_height_ = 0;
}

bool Renderer::CreateBackBuffer(std::wstring& error) {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    if (FAILED(hr)) {
        error = HresultMessage(L"スワップチェーンのバックバッファを取得できませんでした。", hr);
        return false;
    }
    hr = device_->CreateRenderTargetView(back_buffer.Get(), nullptr, backbuffer_rtv_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"バックバッファのレンダーターゲットを作成できませんでした。", hr);
        return false;
    }
    return true;
}

void Renderer::ReleaseBackBuffer() {
    if (context_) {
        context_->OMSetRenderTargets(0, nullptr, nullptr);
    }
    backbuffer_rtv_.Reset();
}

bool Renderer::CreatePipeline(std::wstring& error) {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
    vertices.reserve(std::size(kFaces) * 4);
    indices.reserve(std::size(kFaces) * 6);
    for (const Face& face : kFaces) {
        const auto base = static_cast<std::uint16_t>(vertices.size());
        for (const auto& corner : face.corners) {
            vertices.push_back(Vertex{
                {corner[0], corner[1], corner[2]},
                {face.normal[0], face.normal[1], face.normal[2]},
            });
        }
        const std::uint16_t quad[] = {
            base,
            static_cast<std::uint16_t>(base + 1),
            static_cast<std::uint16_t>(base + 2),
            base,
            static_cast<std::uint16_t>(base + 2),
            static_cast<std::uint16_t>(base + 3),
        };
        indices.insert(indices.end(), std::begin(quad), std::end(quad));
    }
    index_count_ = static_cast<UINT>(indices.size());

    D3D11_BUFFER_DESC vertex_desc{};
    vertex_desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
    vertex_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vertex_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertex_data{};
    vertex_data.pSysMem = vertices.data();
    HRESULT hr = device_->CreateBuffer(&vertex_desc, &vertex_data, vertex_buffer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"頂点バッファを作成できませんでした。", hr);
        return false;
    }

    D3D11_BUFFER_DESC index_desc{};
    index_desc.ByteWidth = static_cast<UINT>(sizeof(std::uint16_t) * indices.size());
    index_desc.Usage = D3D11_USAGE_IMMUTABLE;
    index_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA index_data{};
    index_data.pSysMem = indices.data();
    hr = device_->CreateBuffer(&index_desc, &index_data, index_buffer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"インデックスバッファを作成できませんでした。", hr);
        return false;
    }

    // Same winding as the cube's top face. Y is just below the cube so the bottom face does not z-fight.
    constexpr float kFloorY = -0.501f;
    constexpr float kFloorHalf = 20.0f;
    const Vertex floor_vertices[] = {
        {{-kFloorHalf, kFloorY, kFloorHalf}, {0.0f, 1.0f, 0.0f}},
        {{kFloorHalf, kFloorY, kFloorHalf}, {0.0f, 1.0f, 0.0f}},
        {{kFloorHalf, kFloorY, -kFloorHalf}, {0.0f, 1.0f, 0.0f}},
        {{-kFloorHalf, kFloorY, -kFloorHalf}, {0.0f, 1.0f, 0.0f}},
    };
    const std::uint16_t floor_indices[] = {0, 1, 2, 0, 2, 3};
    floor_index_count_ = static_cast<UINT>(std::size(floor_indices));

    D3D11_BUFFER_DESC floor_vertex_desc{};
    floor_vertex_desc.ByteWidth = sizeof(floor_vertices);
    floor_vertex_desc.Usage = D3D11_USAGE_IMMUTABLE;
    floor_vertex_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA floor_vertex_data{};
    floor_vertex_data.pSysMem = floor_vertices;
    hr = device_->CreateBuffer(&floor_vertex_desc, &floor_vertex_data, floor_vertex_buffer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"床の頂点バッファを作成できませんでした。", hr);
        return false;
    }

    D3D11_BUFFER_DESC floor_index_desc{};
    floor_index_desc.ByteWidth = sizeof(floor_indices);
    floor_index_desc.Usage = D3D11_USAGE_IMMUTABLE;
    floor_index_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA floor_index_data{};
    floor_index_data.pSysMem = floor_indices;
    hr = device_->CreateBuffer(&floor_index_desc, &floor_index_data, floor_index_buffer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"床のインデックスバッファを作成できませんでした。", hr);
        return false;
    }

    D3D11_BUFFER_DESC constant_desc{};
    constant_desc.ByteWidth = sizeof(FrameConstants);
    constant_desc.Usage = D3D11_USAGE_DYNAMIC;
    constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device_->CreateBuffer(&constant_desc, nullptr, constant_buffer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"定数バッファを作成できませんでした。", hr);
        return false;
    }

    D3D11_RASTERIZER_DESC raster_desc{};
    raster_desc.FillMode = D3D11_FILL_SOLID;
    raster_desc.CullMode = D3D11_CULL_BACK;
    raster_desc.FrontCounterClockwise = FALSE;
    raster_desc.DepthClipEnable = TRUE;
    hr = device_->CreateRasterizerState(&raster_desc, rasterizer_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"ラスタライザステートを作成できませんでした。", hr);
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depth_desc{};
    depth_desc.DepthEnable = TRUE;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_desc.DepthFunc = D3D11_COMPARISON_LESS;
    hr = device_->CreateDepthStencilState(&depth_desc, depth_state_.GetAddressOf());
    if (FAILED(hr)) {
        error = HresultMessage(L"深度ステンシルステートを作成できませんでした。", hr);
        return false;
    }
    return true;
}

bool Renderer::CompileShaders(std::string& error) {
    const std::filesystem::path shader_path = ExecutableDirectory() / L"lit.hlsl";
    const std::string source = ReadTextFile(shader_path, error);
    if (source.empty()) {
        if (error.empty()) {
            error = "lit.hlsl is empty.";
        }
        return false;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> vertex_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixel_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> compiler_errors;

    HRESULT hr = D3DCompile(
        source.data(),
        source.size(),
        "lit.hlsl",
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        vertex_blob.GetAddressOf(),
        compiler_errors.GetAddressOf());
    if (FAILED(hr)) {
        error = BlobMessage(compiler_errors.Get());
        if (error.empty()) {
            error = "D3DCompile failed for VSMain.";
        }
        return false;
    }

    compiler_errors.Reset();
    hr = D3DCompile(
        source.data(),
        source.size(),
        "lit.hlsl",
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        pixel_blob.GetAddressOf(),
        compiler_errors.GetAddressOf());
    if (FAILED(hr)) {
        error = BlobMessage(compiler_errors.Get());
        if (error.empty()) {
            error = "D3DCompile failed for PSMain.";
        }
        return false;
    }

    hr = device_->CreateVertexShader(
        vertex_blob->GetBufferPointer(),
        vertex_blob->GetBufferSize(),
        nullptr,
        vertex_shader_.GetAddressOf());
    if (FAILED(hr)) {
        error = "CreateVertexShader failed.";
        return false;
    }
    hr = device_->CreatePixelShader(
        pixel_blob->GetBufferPointer(),
        pixel_blob->GetBufferSize(),
        nullptr,
        pixel_shader_.GetAddressOf());
    if (FAILED(hr)) {
        error = "CreatePixelShader failed.";
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device_->CreateInputLayout(
        layout,
        2,
        vertex_blob->GetBufferPointer(),
        vertex_blob->GetBufferSize(),
        input_layout_.GetAddressOf());
    if (FAILED(hr)) {
        error = "CreateInputLayout failed.";
        return false;
    }
    return true;
}

bool Renderer::EnsureSceneTarget(UINT width, UINT height) {
    if (scene_rtv_ && scene_srv_ && scene_dsv_ && width == scene_width_ && height == scene_height_) {
        return true;
    }

    scene_dsv_.Reset();
    scene_depth_.Reset();
    scene_srv_.Reset();
    scene_rtv_.Reset();
    scene_color_.Reset();
    scene_width_ = 0;
    scene_height_ = 0;

    D3D11_TEXTURE2D_DESC color_desc{};
    color_desc.Width = width;
    color_desc.Height = height;
    color_desc.MipLevels = 1;
    color_desc.ArraySize = 1;
    color_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    color_desc.SampleDesc.Count = 1;
    color_desc.Usage = D3D11_USAGE_DEFAULT;
    color_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device_->CreateTexture2D(&color_desc, nullptr, scene_color_.GetAddressOf())) ||
        FAILED(device_->CreateRenderTargetView(scene_color_.Get(), nullptr, scene_rtv_.GetAddressOf())) ||
        FAILED(device_->CreateShaderResourceView(scene_color_.Get(), nullptr, scene_srv_.GetAddressOf()))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC depth_desc = color_desc;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.Format = depth_desc.Format;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(device_->CreateTexture2D(&depth_desc, nullptr, scene_depth_.GetAddressOf())) ||
        FAILED(device_->CreateDepthStencilView(scene_depth_.Get(), &dsv_desc, scene_dsv_.GetAddressOf()))) {
        return false;
    }

    scene_width_ = width;
    scene_height_ = height;
    return true;
}

void Renderer::UnbindTargets() {
    if (!context_) {
        return;
    }
    ID3D11RenderTargetView* unbound = nullptr;
    context_->OMSetRenderTargets(1, &unbound, nullptr);
}
