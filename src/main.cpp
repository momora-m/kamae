#include "attack.hpp"
#include "attack_mark.hpp"
#include "controller.hpp"
#include "renderer.hpp"
#include "subject_panel.hpp"
#include "trial.hpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <filesystem>
#include <string>
#include <system_error>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace {

constexpr float kMaxFrameSeconds = 0.1f;

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

float FrameSeconds() {
    static LARGE_INTEGER frequency{};
    static LARGE_INTEGER previous{};
    static bool started = false;
    if (!started) {
        if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
            return 0.0f;
        }
        QueryPerformanceCounter(&previous);
        started = true;
        return 0.0f;
    }

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    const float seconds = static_cast<float>(now.QuadPart - previous.QuadPart) /
                          static_cast<float>(frequency.QuadPart);
    previous = now;
    if (seconds < 0.0f) {
        return 0.0f;
    }
    return std::min(seconds, kMaxFrameSeconds);
}

void ApplyDefaultDockLayout(ImGuiID dockspace_id, ImVec2 node_size) {
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, node_size);

    ImGuiID viewport_node = 0;
    ImGuiID properties = 0;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.30f, &properties, &viewport_node);

    ImGui::DockBuilderDockWindow("Viewport", viewport_node);
    ImGui::DockBuilderDockWindow("Subjects", properties);
    ImGui::DockBuilderDockWindow("Camera", properties);
    ImGui::DockBuilderDockWindow("Render", properties);
    // A window tab id is GetID("#TAB"), seeded by the window id. Set it before
    // Finish so the new tab bar opens on Subjects.
    if (ImGuiDockNode* properties_node = ImGui::DockBuilderGetNode(properties)) {
        properties_node->SelectedTabId = ImHashStr("#TAB", 0, ImHashStr("Subjects"));
    }
    ImGui::DockBuilderFinish(dockspace_id);
    ImGui::SetWindowFocus("Subjects");
}

void ShowScenePanels(Renderer& renderer, SceneState& scene, float frame_seconds) {
    // The bar reserves the top of the main viewport work area. EditorHost is
    // placed on that work area, so the dock and the viewport fill below it.
    // The menu sets reset_layout for the next frame, when this host can remove
    // the dock node and build the default layout again.
    static bool reset_layout = false;
    const bool rebuild_layout = reset_layout;
    reset_layout = false;
    ShowSceneMenu(scene, &reset_layout);

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
    if (apply_default_layout || rebuild_layout) {
        const ImVec2 node_size = ImGui::GetWindowSize();
        if (node_size.x >= 1.0f && node_size.y >= 1.0f) {
            if (rebuild_layout) {
                ImGui::DockBuilderRemoveNode(dockspace_id);
            }
            apply_default_layout = false;
            ApplyDefaultDockLayout(dockspace_id, node_size);
        } else if (rebuild_layout) {
            reset_layout = true;
        }
    }
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
    ImGui::End();

    bool yaw_held[kSubjectCapacity]{};
    ShowSubjectPanel(scene, yaw_held, kSubjectCapacity);

    ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::SliderFloat(
        "Distance", &scene.camera_distance, kCameraDistanceMin, kCameraDistanceMax, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("Yaw %.1f deg", ActiveCameraYaw(scene) * (180.0f / std::numbers::pi_v<float>));
    ImGui::Text("Pitch %.1f deg", ActiveCameraPitch(scene) * (180.0f / std::numbers::pi_v<float>));
    ImGui::TextWrapped(
        "In edit mode, left-drag orbits and stays. During play and after a stop, the camera sits behind the player. While WASD is held, that back stays on the yaw from before the move. Releasing the keys leaves that follow yaw in place. Left-drag yaw and pitch stay after the button is released, and the next move uses that horizontal yaw.");
    ImGui::TextWrapped(
        "During a trial, hover the viewport and press WASD. Movement uses the camera's horizontal yaw, including a look that stays after the button is released. Pitch is ignored. While moving, the body faces that direction: W along the look, A to the camera's left, D to its right, and S toward the camera so the face looks at it. Diagonals face that frame's move. Releasing the keys keeps the yaw and puts the camera on that back. Attacks fire along the body's yaw. Q jumps once, and only on the floor. The center rises 0.5. Airborne subjects cannot jump. Opponents do not jump. E dodges once, and only on the floor. It sets horizontal speed 10 for 6 frames. Hits in those frames do not reduce remaining, do not start hitstop, and do not add velocity. Walk, attacks, guard, and jump do not start during a dodge. The gun and devour do not start either. R still switches during a dodge, but not on the frame E is pressed. An in-progress swing keeps going. Opponents do not dodge. Startup is 1 frame, then the hit box is out for 3 active frames, then recovery is 1 frame. Startup and recovery lock walk and a new move. Hold Shift to guard. Guarding locks walk and a new move. A frontal hit while guarding does not reduce remaining, does not start hitstop, and does not add velocity. Side and back still hit. An unblocked hit adds speed 1.0 along the volume's forward. One swing hits a subject once, even when blocked. On the floor, a walk replaces horizontal velocity. Airborne subjects cannot walk. Gravity 10 pulls down; landing clears Y velocity. A blocked axis also clears that velocity. The opponent walks in off the player's front, waits 0.5 seconds after a move's hit box overlaps, attacks with the short move when it reaches and the long move only when the short one does not, or guards instead when the player's poke or long box overlaps on the fire frame, then walks back until that hit box misses. The god arc starts on the blade. R switches blade and gun once. On the blade, Space is the short move (forward 1.0, every 0.4 seconds) and F is the long move (forward 2.0, every 1.0 seconds). Space and F do nothing on the gun. G fires the gun (forward 4.0, every 0.5 seconds) and spends one round. G does nothing on the blade, and does nothing at 0 rounds. Play starts with 3 rounds. C devours on the blade and the gun (forward 0.6, every 0.6 seconds). The hit box uses the same lifetime. A connected devour restores one round, and never past 3. A whiff, a dodge, or a frontal guard does not restore a round. Devour does not reduce remaining, does not start hitstop, and does not add velocity. C together with Space, F, or G devours and does not swing. On the floor, E together with C dodges only. Opponents do not devour. The viewport shows the form and the rounds. R together with an attack uses the form after the switch. Opponents stay on the blade. A press in the last 0.15 seconds keeps only the last move that this form can fire. Nothing walks, switches, or attacks until Start. A subject that was hit passes 0 seconds into walk and attack for the next 4 frames. Other subjects keep moving. The hit box still counts down each frame.");
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
    if (!renderer.MeshError().empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", renderer.MeshError().c_str());
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

    if (scene.session == SessionMode::Editing) {
        scene.camera_yaw_offset = 0.0f;
        scene.camera_pitch_offset = 0.0f;
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            scene.camera_yaw -= delta.x * 0.008f;
            scene.camera_pitch = std::clamp(
                scene.camera_pitch - delta.y * 0.008f, -kCameraPitchLimit, kCameraPitchLimit);
        }
    } else if (ImGui::IsItemActive()) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        scene.camera_yaw_offset -= delta.x * 0.008f;
        const float pitch = std::clamp(
            scene.camera_pitch + scene.camera_pitch_offset - delta.y * 0.008f,
            -kCameraPitchLimit,
            kCameraPitchLimit);
        scene.camera_pitch_offset = pitch - scene.camera_pitch;
    }
    const bool viewport_hovered = ImGui::IsItemHovered();
    float subject_seconds[kSubjectCapacity]{};
    if (scene.session == SessionMode::Trial) {
        for (int index = 0; index < scene.subject_count; ++index) {
            subject_seconds[index] = TakeSubjectFrameSeconds(scene.subjects[index], frame_seconds);
        }
    }
    if (scene.session == SessionMode::Trial || scene.session == SessionMode::Stopped) {
        TickAttackVolumes(scene.volumes, scene.volume_count);
        if (scene.session == SessionMode::Trial) {
            TryStartPlayerDodge(scene, subject_seconds[kPlayer], viewport_hovered);
        }
        ApplyVolumeHits(scene);
    }
    if (scene.session == SessionMode::Trial) {
        for (int index = 0; index < scene.subject_count; ++index) {
            StepController(
                scene,
                scene.controllers[index],
                subject_seconds[index],
                viewport_hovered,
                yaw_held[index]);
        }
        ApplyVolumeHits(scene);
        for (int index = 0; index < scene.subject_count; ++index) {
            IntegrateAndResolve(scene, index, subject_seconds[index]);
        }
        TickDodgeFrames(scene);
        if (TrialStopReason(scene) != TrialStop::Continue) {
            scene.session = SessionMode::Stopped;
        }
    } else if (scene.session == SessionMode::Editing) {
        ClearAttackVolumes(scene.volumes, scene.volume_count);
    }

    const auto target_width = static_cast<UINT>(view_size.x);
    const auto target_height = static_cast<UINT>(view_size.y);
    renderer.DrawScene(scene, target_width, target_height);

    const ImTextureID scene_texture = renderer.SceneColorTexture();
    if (scene_texture != 0) {
        const ImTextureRef texture(scene_texture);
        ImGui::GetWindowDrawList()->AddImage(
            texture,
            view_origin,
            ImVec2(view_origin.x + view_size.x, view_origin.y + view_size.y));
    }
    const float line_height = ImGui::GetTextLineHeight();
    for (int index = 0; index < scene.subject_count; ++index) {
        char remaining_line[96];
        if (index == kPlayer) {
            const char* form = scene.subjects[index].weapon_form == WeaponForm::Gun ? "gun" : "blade";
            std::snprintf(
                remaining_line,
                sizeof(remaining_line),
                "Subject %d remaining %d  %s  rounds %d",
                index,
                scene.subjects[index].remaining,
                form,
                scene.subjects[index].rounds);
        } else {
            std::snprintf(
                remaining_line,
                sizeof(remaining_line),
                "Subject %d remaining %d",
                index,
                scene.subjects[index].remaining);
        }
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(view_origin.x + 8.0f, view_origin.y + 8.0f + line_height * static_cast<float>(index)),
            IM_COL32(236, 232, 223, 255),
            remaining_line);
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
        const float frame_seconds = FrameSeconds();
        if (!renderer.PrepareFrame()) {
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ShowScenePanels(renderer, scene, frame_seconds);
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
