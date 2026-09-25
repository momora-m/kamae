#include "attack.hpp"

#include "attack_mark.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr float kAttackForward = 1.0f;
constexpr float kAttackLateral = 0.5f;
constexpr float kAttackInterval = 0.4f;

}  // namespace

// Yaw 0 faces +Z. The volume starts at the front face and extends kAttackForward,
// with lateral half-width kAttackLateral. Off-axis yaw uses the corners' bounds.
AxisBox AttackBox(const Subject& attacker) {
    const float yaw = attacker.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    const float forward_x = std::sin(yaw);
    const float forward_z = std::cos(yaw);
    const float right_x = std::cos(yaw);
    const float right_z = -std::sin(yaw);
    const float forward_steps[2] = {kCubeHalfExtent, kCubeHalfExtent + kAttackForward};
    const float lateral_steps[2] = {-kAttackLateral, kAttackLateral};

    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
    for (int forward_index = 0; forward_index < 2; ++forward_index) {
        for (int lateral_index = 0; lateral_index < 2; ++lateral_index) {
            const float x = attacker.position[0] + forward_x * forward_steps[forward_index] +
                            right_x * lateral_steps[lateral_index];
            const float z = attacker.position[2] + forward_z * forward_steps[forward_index] +
                            right_z * lateral_steps[lateral_index];
            if (forward_index == 0 && lateral_index == 0) {
                min_x = max_x = x;
                min_z = max_z = z;
                continue;
            }
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
            min_z = std::min(min_z, z);
            max_z = std::max(max_z, z);
        }
    }

    return AxisBox{
        min_x,
        attacker.position[1] - kCubeHalfExtent,
        min_z,
        max_x,
        attacker.position[1] + kCubeHalfExtent,
        max_z,
    };
}

void Attack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    bool attack_pressed,
    AttackMark* mark) {
    if (mark != nullptr) {
        ClearAttackMark(*mark);
    }
    if (subjects == nullptr || attacker_index < 0 || attacker_index >= subject_count) {
        return;
    }

    Subject& attacker = subjects[attacker_index];
    if (frame_seconds > 0.0f) {
        attacker.attack_cooldown = std::max(0.0f, attacker.attack_cooldown - frame_seconds);
    }
    if (attacker.remaining <= 0 || !attack_pressed) {
        return;
    }
    if (attacker.attack_cooldown > 0.0f) {
        return;
    }
    attacker.attack_cooldown = kAttackInterval;

    const AxisBox hit = AttackBox(attacker);
    if (mark != nullptr) {
        ShowAttackMark(*mark, hit);
    }
    for (int index = 0; index < subject_count; ++index) {
        if (index == attacker_index) {
            continue;
        }
        Subject& other = subjects[index];
        if (other.remaining <= 0) {
            continue;
        }
        if (!AxisBoxesOverlap(hit, SubjectBox(other))) {
            continue;
        }
        other.remaining -= 1;
    }
}
