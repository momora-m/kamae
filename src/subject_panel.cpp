#include "subject_panel.hpp"

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
    ImGui::End();
}
