#include "walk.hpp"

#include "renderer.hpp"

#include "imgui.h"

#include <cmath>

namespace {

constexpr float kWalkSpeed = 2.5f;

}  // namespace

void WalkCube(SceneState& scene, const HorizontalBasis& basis, float frame_seconds, bool viewport_hovered) {
    if (!viewport_hovered || ImGui::GetIO().WantTextInput || frame_seconds <= 0.0f) {
        return;
    }

    float strafe = 0.0f;
    float forward_input = 0.0f;
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
        strafe += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
        strafe -= 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
        forward_input += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S)) {
        forward_input -= 1.0f;
    }
    const float length = std::sqrt(strafe * strafe + forward_input * forward_input);
    if (length < 0.001f) {
        return;
    }
    strafe /= length;
    forward_input /= length;

    const float distance = kWalkSpeed * frame_seconds;
    const float move_x = (basis.right_x * strafe + basis.forward_x * forward_input) * distance;
    const float move_z = (basis.right_z * strafe + basis.forward_z * forward_input) * distance;
    scene.cube_position[0] += move_x;
    scene.cube_position[2] += move_z;
}
