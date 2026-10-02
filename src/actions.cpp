#include "actions.hpp"

#include "imgui.h"

SubjectActions PlayerActionsFromKeys(bool viewport_hovered) {
    if (!viewport_hovered || ImGui::GetIO().WantTextInput) {
        return SubjectActions{HorizontalWalk{0.0f, 0.0f}, kMoveNone};
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
    int move = kMoveNone;
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        move = kMovePoke;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        move = kMoveLong;
    }
    const bool guard = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    const bool jump = ImGui::IsKeyPressed(ImGuiKey_Q, false);
    const bool dodge = ImGui::IsKeyPressed(ImGuiKey_E, false);
    // R and G stay free of WASD, Space, F, Shift, Q, and E. The session
    // drops the attack that does not match the form after R.
    const bool fire_gun = ImGui::IsKeyPressed(ImGuiKey_G, false);
    const bool switch_form = ImGui::IsKeyPressed(ImGuiKey_R, false);
    return SubjectActions{HorizontalWalk{strafe, forward}, move, guard, jump, dodge, fire_gun, switch_form};
}
