#include "renderer.hpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

// imgui_impl_win32.h wraps this prototype in #if 0. Keep the declaration at
// global scope so WndProc binds to imgui_impl_win32.cpp, not an internal symbol.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kPitchLimit = 1.48f;

Renderer* g_renderer = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_SIZE && wparam != SIZE_MINIMIZED && g_renderer != nullptr) {
        g_renderer->RequestResize(static_cast<UINT>(LOWORD(lparam)), static_cast<UINT>(HIWORD(lparam)));
    }

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return 1;
    }

    switch (msg) {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU) {
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void ApplyDefaultDockLayout(ImGuiID dockspace_id, ImVec2 node_size) {
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, node_size);

    ImGuiID viewport_node = 0;
    ImGuiID properties = 0;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.30f, &properties, &viewport_node);

    ImGuiID cube_node = 0;
    ImGuiID lower = 0;
    ImGui::DockBuilderSplitNode(properties, ImGuiDir_Down, 0.64f, &lower, &cube_node);

    ImGuiID camera_node = 0;
    ImGuiID render_node = 0;
    ImGui::DockBuilderSplitNode(lower, ImGuiDir_Down, 0.50f, &render_node, &camera_node);

    ImGui::DockBuilderDockWindow("Viewport", viewport_node);
    ImGui::DockBuilderDockWindow("Cube", cube_node);
    ImGui::DockBuilderDockWindow("Camera", camera_node);
    ImGui::DockBuilderDockWindow("Render", render_node);
    ImGui::DockBuilderFinish(dockspace_id);
}

void ShowScenePanels(Renderer& renderer, SceneState& scene) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("EditorHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
    std::error_code ini_error;
    static bool apply_default_layout = !std::filesystem::exists("editor_layout.ini", ini_error);
    if (apply_default_layout) {
        const ImVec2 node_size = ImGui::GetWindowSize();
        if (node_size.x >= 1.0f && node_size.y >= 1.0f) {
            apply_default_layout = false;
            ApplyDefaultDockLayout(dockspace_id, node_size);
        }
    }
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
    ImGui::End();

    ImGui::Begin("Cube", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextUnformatted("World position");
    ImGui::DragFloat3("Position", scene.cube_position, 0.01f);
    ImGui::TextUnformatted("Euler rotation in degrees");
    ImGui::DragFloat3("Rotation", scene.cube_rotation_degrees, 0.5f);
    ImGui::End();

    ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::SliderFloat("Distance", &scene.camera_distance, 1.5f, 20.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("Yaw %.1f deg", scene.camera_yaw * (180.0f / kPi));
    ImGui::Text("Pitch %.1f deg", scene.camera_pitch * (180.0f / kPi));
    ImGui::TextWrapped("Left-drag inside the viewport to orbit around the cube.");
    ImGui::End();

    ImGui::Begin("Render", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::ColorEdit3("Clear color", scene.clear_color);
    const float fps = ImGui::GetIO().Framerate;
    if (fps > 0.0f) {
        ImGui::Text("Frame %.2f ms (%.0f FPS)", 1000.0f / fps, fps);
    }
    if (!renderer.ShaderError().empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", renderer.ShaderError().c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::Begin(
        "Viewport",
        nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (!renderer.ShaderError().empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "Shader failed. See the Render panel.");
    }

    ImVec2 view_size = ImGui::GetContentRegionAvail();
    view_size.x = std::max(view_size.x, 1.0f);
    view_size.y = std::max(view_size.y, 1.0f);
    const ImVec2 view_origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##viewport", view_size);

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        scene.camera_yaw -= delta.x * 0.008f;
        scene.camera_pitch = std::clamp(scene.camera_pitch - delta.y * 0.008f, -kPitchLimit, kPitchLimit);
    }

    const auto target_width = static_cast<UINT>(view_size.x);
    const auto target_height = static_cast<UINT>(view_size.y);
    renderer.DrawScene(scene, target_width, target_height);

    ID3D11ShaderResourceView* scene_texture = renderer.SceneColorSrv();
    if (scene_texture != nullptr) {
        const ImTextureRef texture(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(scene_texture)));
        ImGui::GetWindowDrawList()->AddImage(
            texture,
            view_origin,
            ImVec2(view_origin.x + view_size.x, view_origin.y + view_size.y));
    }
    ImGui::End();
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show_command) {
    ImGui_ImplWin32_EnableDpiAwareness();
    const float dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
        MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = WndProc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = L"EditorWindowClass";
    if (RegisterClassExW(&window_class) == 0) {
        MessageBoxW(nullptr, L"ウィンドウクラスを登録できませんでした。", L"editor", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int width = static_cast<int>(1440.0f * dpi_scale);
    const int height = static_cast<int>(900.0f * dpi_scale);
    HWND hwnd = CreateWindowExW(
        0,
        window_class.lpszClassName,
        L"editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (hwnd == nullptr) {
        UnregisterClassW(window_class.lpszClassName, instance);
        MessageBoxW(nullptr, L"ウィンドウを作成できませんでした。", L"editor", MB_OK | MB_ICONERROR);
        return 1;
    }

    Renderer renderer;
    std::wstring error;
    if (!renderer.Initialize(hwnd, error)) {
        if (error.empty()) {
            error = L"DirectX 11 の初期化に失敗しました。";
        }
        MessageBoxW(nullptr, error.c_str(), L"editor", MB_OK | MB_ICONERROR);
        DestroyWindow(hwnd);
        UnregisterClassW(window_class.lpszClassName, instance);
        return 1;
    }
    g_renderer = &renderer;

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "editor_layout.ini";

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.ScaleAllSizes(dpi_scale);
    style.FontScaleDpi = dpi_scale;

    if (!ImGui_ImplWin32_Init(hwnd) || !ImGui_ImplDX11_Init(renderer.Device(), renderer.Context())) {
        MessageBoxW(nullptr, L"Dear ImGui を初期化できませんでした。", L"editor", MB_OK | MB_ICONERROR);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_renderer = nullptr;
        renderer.Shutdown();
        DestroyWindow(hwnd);
        UnregisterClassW(window_class.lpszClassName, instance);
        return 1;
    }

    SceneState scene;
    bool done = false;
    while (!done) {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (message.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }
        if (!renderer.PrepareFrame()) {
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ShowScenePanels(renderer, scene);
        ImGui::Render();

        renderer.BindAndClearBackBuffer();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        renderer.Present();

        if (renderer.DeviceLost()) {
            MessageBoxW(nullptr, L"DirectX 11 デバイスが失われました。", L"editor", MB_OK | MB_ICONERROR);
            break;
        }
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_renderer = nullptr;
    renderer.Shutdown();
    DestroyWindow(hwnd);
    UnregisterClassW(window_class.lpszClassName, instance);
    return 0;
}
