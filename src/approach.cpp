#include "approach.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "overlap.hpp"
#include "renderer.hpp"
#include "walk.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr float kDirectionEpsilon = 0.001f;

}  // namespace

void ApproachAndAttack(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    AttackMark* mark) {
    if (subjects == nullptr || mover_index < 0 || target_index < 0 || mover_index >= subject_count ||
        target_index >= subject_count || mover_index == target_index || subjects[mover_index].remaining <= 0 ||
        subjects[target_index].remaining <= 0) {
        if (mark != nullptr) {
            ClearAttackMark(*mark);
        }
        return;
    }

    Subject& mover = subjects[mover_index];
    const Subject& target = subjects[target_index];
    const float dx = target.position[0] - mover.position[0];
    const float dz = target.position[2] - mover.position[2];
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length >= kDirectionEpsilon) {
        const float yaw_degrees = std::atan2(dx, dz) * (180.0f / std::numbers::pi_v<float>);
        Subject aimed = mover;
        aimed.rotation_degrees[1] = yaw_degrees;
        if (AxisBoxesOverlap(AttackBox(aimed), SubjectBox(target))) {
            mover.rotation_degrees[1] = yaw_degrees;
            Attack(subjects, subject_count, mover_index, frame_seconds, true, mark);
            return;
        }
        if (frame_seconds > 0.0f) {
            const float previous_x = mover.position[0];
            const float previous_z = mover.position[2];
            const float distance = std::min(kWalkSpeed * frame_seconds, length);
            mover.position[0] += (dx / length) * distance;
            mover.position[2] += (dz / length) * distance;
            mover.rotation_degrees[1] = yaw_degrees;
            ResolveHorizontalOverlap(subjects, subject_count, mover_index, previous_x, previous_z);
        }
    }

    Attack(subjects, subject_count, mover_index, frame_seconds, false, mark);
}
