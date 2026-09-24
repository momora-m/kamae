#include "subject_panel.hpp"

#include "renderer.hpp"

#include "imgui.h"

void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count) {
    if (yaw_held != nullptr) {
        for (int index = 0; index < yaw_held_count; ++index) {
            yaw_held[index] = false;
        }
    }

    ImGui::Begin("Subjects", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextUnformatted("The first subject is the player. Roles stay on the list.");
    for (int index = 0; index < kSubjectCount; ++index) {
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
