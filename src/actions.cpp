#include "actions.hpp"

#include "imgui.h"

SubjectActions PlayerActionsFromKeys(bool viewport_hovered) {
    if (!viewport_hovered || ImGui::GetIO().WantTextInput) {
        return SubjectActions{HorizontalWalk{0.0f, 0.0f}, false};
    }

    float strafe = 0.0f;
    float forward = 0.0f;
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
        strafe += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
        strafe -= 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
        forward += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S)) {
        forward -= 1.0f;
    }
    const bool attack = ImGui::IsKeyPressed(ImGuiKey_Space, false);
    return SubjectActions{HorizontalWalk{strafe, forward}, attack};
}
