#include "subject_panel.hpp"

#include "arena_file.hpp"
#include "renderer.hpp"
#include "roster.hpp"
#include "trial.hpp"

#include "imgui.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

void CopySceneName(char* dest, int dest_size, const std::string& name) {
    if (dest == nullptr || dest_size <= 0) {
        return;
    }
    std::snprintf(dest, static_cast<std::size_t>(dest_size), "%s", name.c_str());
}

}  // namespace

void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count) {
    if (yaw_held != nullptr) {
        for (int index = 0; index < yaw_held_count; ++index) {
            yaw_held[index] = false;
        }
    }

    ImGui::Begin("Subjects", nullptr, ImGuiWindowFlags_NoCollapse);
    if (scene.session == SessionMode::Editing && ImGui::Button("Start")) {
        BeginTrial(scene);
    }
    if (scene.session == SessionMode::Stopped) {
        ImGui::TextUnformatted("Trial stopped. The layout stays until you return.");
        if (ImGui::Button("Return to start")) {
            RestoreStartLayout(scene);
        }
        ImGui::End();
        return;
    }
    if (scene.session != SessionMode::Editing) {
        ImGui::TextUnformatted("Trial. Layout edits stay hidden.");
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("Editing. Time is stopped until Start.");
    ImGui::TextUnformatted("The first subject is the player. Roles stay on the list.");
    const bool can_add = CanAddSubject(scene);
    if (!can_add) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Add opponent")) {
        AddSubject(scene);
    }
    if (!can_add) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    const bool can_remove = CanRemoveSubject(scene);
    if (!can_remove) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Remove last")) {
        RemoveLastSubject(scene);
    }
    if (!can_remove) {
        ImGui::EndDisabled();
    }
    for (int index = 0; index < scene.subject_count; ++index) {
        ImGui::PushID(index);
        if (index > 0) {
            ImGui::Separator();
        }
        ImGui::Text("Subject %d", index);
        ImGui::DragFloat3("Position", scene.subjects[index].position, 0.01f);
        if (index == kPlayer) {
            ImGui::TextUnformatted("Euler rotation in degrees");
            ImGui::DragFloat3("Rotation", scene.subjects[index].rotation_degrees, 0.5f);
        } else {
            ImGui::DragFloat("Yaw", &scene.subjects[index].rotation_degrees[1], 0.5f);
            if (yaw_held != nullptr && index < yaw_held_count && ImGui::IsItemActive()) {
                yaw_held[index] = true;
            }
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    ImGui::SliderFloat(
        "Floor half",
        &scene.floor_half,
        kFloorHalfMin,
        kFloorHalfMax,
        "%.2f",
        ImGuiSliderFlags_AlwaysClamp);
    if (!scene.layout_error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", scene.layout_error.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void ShowSceneMenu(SceneState& scene) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("Scene", scene.session == SessionMode::Editing)) {
        if (ImGui::MenuItem("Reset to initial")) {
            ApplyBuiltinLayout(scene);
        }
        static char scene_name[kSceneNameMax + 1] = "";
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
        ImGui::InputText("Scene name", scene_name, sizeof(scene_name));
        if (ImGui::MenuItem("Save scene")) {
            SaveScene(scene, scene_name);
        }
        std::vector<std::string> names;
        ListScenes(names);
        ImGui::Separator();
        ImGui::TextUnformatted("Scenes");
        if (names.empty()) {
            ImGui::TextUnformatted("No saved scenes.");
        }
        ImGui::PushID("scene-list");
        for (int index = 0; index < static_cast<int>(names.size()); ++index) {
            ImGui::PushID(index);
            const bool chosen = std::strcmp(scene_name, names[static_cast<std::size_t>(index)].c_str()) == 0;
            if (ImGui::MenuItem(names[static_cast<std::size_t>(index)].c_str(), nullptr, chosen)) {
                CopySceneName(
                    scene_name, static_cast<int>(sizeof(scene_name)), names[static_cast<std::size_t>(index)]);
                LoadScene(scene, names[static_cast<std::size_t>(index)]);
            }
            ImGui::PopID();
        }
        ImGui::PopID();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}
