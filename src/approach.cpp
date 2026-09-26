#include "approach.hpp"

#include "attack.hpp"
#include "overlap.hpp"
#include "renderer.hpp"

#include <cmath>
#include <numbers>

namespace {

constexpr float kDirectionEpsilon = 0.001f;
// Poke lateral 0.5 plus the cube half-extent. Past this, the approach is straight.
constexpr float kOffPlayerFront = 1.0f;

float StrafeOffFront(const Subject& mover, const Subject& target) {
    const float yaw = target.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    const float right_x = std::cos(yaw);
    const float right_z = -std::sin(yaw);
    const float dx = mover.position[0] - target.position[0];
    const float dz = mover.position[2] - target.position[2];
    const float offset = dx * right_x + dz * right_z;
    if (std::fabs(offset) >= kOffPlayerFront) {
        return 0.0f;
    }
    const float to_x = -dx;
    const float to_z = -dz;
    const float length = std::sqrt(to_x * to_x + to_z * to_z);
    if (length < kDirectionEpsilon) {
        return 0.0f;
    }
    const HorizontalBasis basis = BasisTowardTarget(to_x, to_z, length);
    const float side = offset >= 0.0f ? 1.0f : -1.0f;
    const float along_right = (right_x * side) * basis.right_x + (right_z * side) * basis.right_z;
    if (std::fabs(along_right) < kDirectionEpsilon) {
        return side;
    }
    return along_right > 0.0f ? 1.0f : -1.0f;
}

}  // namespace

OpponentCommand CommandForOpponentState(OpponentState state) {
    if (state == OpponentState::Fire) {
        return OpponentCommand{state, HorizontalWalk{0.0f, 0.0f}, kMovePoke};
    }
    if (state == OpponentState::Wait) {
        return OpponentCommand{state, HorizontalWalk{0.0f, 0.0f}, kMoveNone};
    }
    return OpponentCommand{OpponentState::Approach, HorizontalWalk{0.0f, 1.0f}, kMoveNone};
}

HorizontalBasis BasisTowardTarget(float dx, float dz, float length) {
    const float forward_x = dx / length;
    const float forward_z = dz / length;
    return HorizontalBasis{
        forward_x,
        forward_z,
        forward_z,
        -forward_x,
    };
}

SubjectActions OpponentActions(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    bool keep_yaw) {
    if (subjects == nullptr || mover_index < 0 || target_index < 0 || mover_index >= subject_count ||
        target_index >= subject_count || mover_index == target_index || subjects[mover_index].remaining <= 0 ||
        subjects[target_index].remaining <= 0) {
        return SubjectActions{HorizontalWalk{0.0f, 0.0f}, kMoveNone};
    }

    Subject& mover = subjects[mover_index];
    const Subject& target = subjects[target_index];
    const float dx = target.position[0] - mover.position[0];
    const float dz = target.position[2] - mover.position[2];
    const float length = std::sqrt(dx * dx + dz * dz);

    const float yaw_degrees =
        keep_yaw ? mover.rotation_degrees[1] : std::atan2(dx, dz) * (180.0f / std::numbers::pi_v<float>);
    Subject aimed = mover;
    aimed.rotation_degrees[1] = yaw_degrees;
    const bool in_range =
        length >= kDirectionEpsilon && AxisBoxesOverlap(AttackBox(aimed, kMovePoke), SubjectBox(target));
    const bool long_range =
        length >= kDirectionEpsilon && AxisBoxesOverlap(AttackBox(aimed, kMoveLong), SubjectBox(target));
    const int retreat_box = mover.retreat_move == kMoveNone ? kMovePoke : mover.retreat_move;
    const bool retreat_overlaps =
        length >= kDirectionEpsilon && AxisBoxesOverlap(AttackBox(aimed, retreat_box), SubjectBox(target));

    if (mover.retreating) {
        if (!retreat_overlaps) {
            mover.retreating = false;
            mover.retreat_move = kMoveNone;
            mover.focus_move = kMoveNone;
            mover.attack_reaction = kAttackReactionIdle;
        } else {
            if (!keep_yaw) {
                mover.rotation_degrees[1] = yaw_degrees;
            }
            mover.attack_reaction = kAttackReactionIdle;
            return SubjectActions{HorizontalWalk{0.0f, -1.0f}, kMoveNone};
        }
    }

    int chosen = kMoveNone;
    if (in_range) {
        chosen = kMovePoke;
    } else if (long_range) {
        chosen = kMoveLong;
    }

    OpponentState state = OpponentState::Approach;
    if (chosen != kMoveNone) {
        if (!keep_yaw) {
            mover.rotation_degrees[1] = yaw_degrees;
        }
        if (mover.focus_move != chosen || mover.attack_reaction < 0.0f) {
            mover.focus_move = chosen;
            mover.attack_reaction = kOpponentReactionDelay;
        }
        mover.attack_reaction = AdvanceAttackTimer(mover.attack_reaction, frame_seconds);
        if (mover.attack_reaction <= 0.0f && mover.attack_cooldown <= 0.0f) {
            mover.retreating = true;
            mover.retreat_move = chosen;
            mover.focus_move = kMoveNone;
            mover.attack_reaction = kAttackReactionIdle;
            state = OpponentState::Fire;
        } else {
            state = OpponentState::Wait;
        }
    }
    if (state == OpponentState::Approach) {
        mover.attack_reaction = kAttackReactionIdle;
        mover.focus_move = kMoveNone;
    }

    OpponentCommand command = CommandForOpponentState(state);
    if (state == OpponentState::Fire) {
        command.move = chosen;
    }
    if (state == OpponentState::Approach) {
        command.walk.strafe = StrafeOffFront(mover, target);
    }
    return SubjectActions{command.walk, command.move};
}
