#include "walk.hpp"

#include "renderer.hpp"

#include <cmath>
#include <numbers>

static_assert(kJumpHeight == kCubeHalfExtent, "the hop rises by one cube half-extent");
static_assert(kGravity == 10.0f, "jump speed is the square root of 2 * 10 * height");
static_assert(kDodgeFrames == 6, "the dodge lasts six trial frames");
static_assert(kDodgeSpeed == 10.0f, "six frames at 60fps travel the short reach of 1");

bool SubjectOnFloor(const Subject& subject) {
    return subject.position[1] <= kFloorCenterY + kFloorEpsilon;
}

void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    const HorizontalWalk& walk,
    float walk_seconds,
    float frame_seconds,
    bool face_move) {
    if (frame_seconds <= 0.0f) {
        return;
    }

    float strafe = walk.strafe;
    float forward_input = walk.forward;
    const float length = std::sqrt(strafe * strafe + forward_input * forward_input);
    if (length < 0.001f) {
        subject.velocity[0] = 0.0f;
        subject.velocity[2] = 0.0f;
        return;
    }
    strafe /= length;
    forward_input /= length;

    const float speed = kWalkSpeed * (walk_seconds / frame_seconds);
    const float move_x = basis.right_x * strafe + basis.forward_x * forward_input;
    const float move_z = basis.right_z * strafe + basis.forward_z * forward_input;
    subject.velocity[0] = move_x * speed;
    subject.velocity[2] = move_z * speed;
    if (face_move) {
        subject.rotation_degrees[1] = std::atan2(move_x, move_z) * (180.0f / std::numbers::pi_v<float>);
    }
}

void IntegrateSubject(Subject& subject, float frame_seconds) {
    if (frame_seconds <= 0.0f) {
        return;
    }
    subject.velocity[1] -= kGravity * frame_seconds;
    subject.position[0] += subject.velocity[0] * frame_seconds;
    subject.position[1] += subject.velocity[1] * frame_seconds;
    subject.position[2] += subject.velocity[2] * frame_seconds;
    if (subject.position[1] < kFloorCenterY) {
        subject.position[1] = kFloorCenterY;
        subject.velocity[1] = 0.0f;
    }
}
