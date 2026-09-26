#include "subject_panel.hpp"

#include "arena_file.hpp"
#include "renderer.hpp"
#include "roster.hpp"

#include "imgui.h"

void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count) {
    if (yaw_held != nullptr) {
        for (int index = 0; index < yaw_held_count; ++index) {
            yaw_held[index] = false;
        }
    }

    ImGui::Begin("Subjects", nullptr, ImGuiWindowFlags_NoCollapse);
    if (scene.session == SessionMode::Editing && ImGui::Button("Start")) {
        scene.session = SessionMode::Trial;
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
    if (ImGui::Button("Save")) {
        SaveArena(scene);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        const ArenaLoad loaded = LoadArena(scene);
        if (loaded == ArenaLoad::Missing) {
            scene.layout_error = "arena.txt was not found.";
        }
    }
    if (!scene.layout_error.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", scene.layout_error.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();
}
