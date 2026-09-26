#include "approach.hpp"

#include "attack.hpp"
#include "overlap.hpp"
#include "renderer.hpp"

#include <cmath>
#include <numbers>

namespace {

constexpr float kDirectionEpsilon = 0.001f;

HorizontalBasis BasisToward(float dx, float dz, float length) {
    const float forward_x = dx / length;
    const float forward_z = dz / length;
    return HorizontalBasis{
        forward_x,
        forward_z,
        forward_z,
        -forward_x,
    };
}

}  // namespace

OpponentCommand CommandForOpponentState(OpponentState state) {
    if (state == OpponentState::Fire) {
        return OpponentCommand{state, HorizontalWalk{0.0f, 0.0f}, true};
    }
    if (state == OpponentState::Wait) {
        return OpponentCommand{state, HorizontalWalk{0.0f, 0.0f}, false};
    }
    return OpponentCommand{OpponentState::Approach, HorizontalWalk{0.0f, 1.0f}, false};
}

void ApproachAndAttack(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    AttackMark* mark,
    bool keep_yaw,
    float floor_half) {
    if (subjects == nullptr || mover_index < 0 || target_index < 0 || mover_index >= subject_count ||
        target_index >= subject_count || mover_index == target_index || subjects[mover_index].remaining <= 0 ||
        subjects[target_index].remaining <= 0) {
        return;
    }

    Subject& mover = subjects[mover_index];
    const Subject& target = subjects[target_index];
    const float dx = target.position[0] - mover.position[0];
    const float dz = target.position[2] - mover.position[2];
    const float length = std::sqrt(dx * dx + dz * dz);

    OpponentState state = OpponentState::Approach;
    if (length >= kDirectionEpsilon) {
        const float yaw_degrees =
            keep_yaw ? mover.rotation_degrees[1]
                     : std::atan2(dx, dz) * (180.0f / std::numbers::pi_v<float>);
        Subject aimed = mover;
        aimed.rotation_degrees[1] = yaw_degrees;
        if (AxisBoxesOverlap(AttackBox(aimed), SubjectBox(target))) {
            mover.rotation_degrees[1] = yaw_degrees;
            if (mover.attack_reaction < 0.0f) {
                mover.attack_reaction = kOpponentReactionDelay;
            }
            mover.attack_reaction = AdvanceAttackTimer(mover.attack_reaction, frame_seconds);
            state = mover.attack_reaction <= 0.0f ? OpponentState::Fire : OpponentState::Wait;
        }
    }
    if (state == OpponentState::Approach) {
        mover.attack_reaction = kAttackReactionIdle;
    }

    const OpponentCommand command = CommandForOpponentState(state);
    if (state == OpponentState::Approach && length >= kDirectionEpsilon && frame_seconds > 0.0f) {
        const float previous_x = mover.position[0];
        const float previous_z = mover.position[2];
        const float step = kWalkSpeed * frame_seconds;
        const float walk_seconds = step > length ? length / kWalkSpeed : frame_seconds;
        WalkCube(mover, BasisToward(dx, dz, length), command.walk, walk_seconds, !keep_yaw);
        ResolveHorizontalOverlap(
            subjects, subject_count, mover_index, previous_x, previous_z, floor_half);
    }

    Attack(
        subjects,
        subject_count,
        mover_index,
        frame_seconds,
        command.attack,
        mark,
        kOpponentAttackInterval,
        false);
}
