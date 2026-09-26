#include "walk.hpp"

#include "renderer.hpp"

#include <cmath>
#include <numbers>

void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    const HorizontalWalk& walk,
    float frame_seconds,
    bool face_move) {
    if (frame_seconds <= 0.0f) {
        return;
    }

    float strafe = walk.strafe;
    float forward_input = walk.forward;
    const float length = std::sqrt(strafe * strafe + forward_input * forward_input);
    if (length < 0.001f) {
        return;
    }
    strafe /= length;
    forward_input /= length;

    const float distance = kWalkSpeed * frame_seconds;
    const float move_x = (basis.right_x * strafe + basis.forward_x * forward_input) * distance;
    const float move_z = (basis.right_z * strafe + basis.forward_z * forward_input) * distance;
    subject.position[0] += move_x;
    subject.position[2] += move_z;
    if (face_move) {
        subject.rotation_degrees[1] = std::atan2(move_x, move_z) * (180.0f / std::numbers::pi_v<float>);
    }
}
