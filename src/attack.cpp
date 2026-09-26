#include "attack.hpp"

#include "actions.hpp"
#include "attack_mark.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr MoveRow kPoke{1.0f, 0.5f, kPlayerAttackInterval};
constexpr MoveRow kLong{kLongMoveForward, 0.5f, kLongMoveInterval};

static_assert(kSubjectCapacity <= 32, "the volume hit mask has one bit per subject");
static_assert(kPoke.interval == 0.4f, "the poke waits 0.4 seconds");
static_assert(kLong.forward > kPoke.forward, "the long move reaches farther");
static_assert(kLong.interval > kPoke.interval, "the long move waits longer");

}  // namespace

static_assert(kAttackReactionIdle < 0.0f, "an idle reaction is not a finished wait");
static_assert(
    Subject{}.attack_reaction == kAttackReactionIdle, "a new subject has not started the in-range wait");

bool TryMove(int move_id, MoveRow& row) {
    if (move_id == kMovePoke) {
        row = kPoke;
        return true;
    }
    if (move_id == kMoveLong) {
        row = kLong;
        return true;
    }
    return false;
}

// Yaw 0 faces +Z. The volume starts at the front face and extends the move's forward
// reach, with the move's lateral half-width. Off-axis yaw uses the corners' bounds.
AxisBox AttackBox(const Subject& attacker, int move_id) {
    MoveRow row{};
    const bool known = TryMove(move_id, row);
    const float forward = known ? row.forward : 0.0f;
    const float lateral = known ? row.lateral : 0.0f;
    const float yaw = attacker.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    const float forward_x = std::sin(yaw);
    const float forward_z = std::cos(yaw);
    const float right_x = std::cos(yaw);
    const float right_z = -std::sin(yaw);
    const float forward_steps[2] = {kCubeHalfExtent, kCubeHalfExtent + forward};
    const float lateral_steps[2] = {-lateral, lateral};

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

void TickAttackVolume(AttackMark& mark) {
    if (mark.remaining_frames <= 0) {
        ClearAttackMark(mark);
        return;
    }
    mark.remaining_frames -= 1;
    if (mark.remaining_frames <= 0) {
        ClearAttackMark(mark);
        return;
    }
    mark.visible = true;
}

void TickAttackVolumes(AttackMark* volumes, int& volume_count) {
    if (volumes == nullptr || volume_count <= 0) {
        volume_count = 0;
        return;
    }
    int kept = 0;
    for (int index = 0; index < volume_count; ++index) {
        TickAttackVolume(volumes[index]);
        if (volumes[index].remaining_frames <= 0) {
            continue;
        }
        if (kept != index) {
            volumes[kept] = volumes[index];
        }
        kept += 1;
    }
    for (int index = kept; index < volume_count; ++index) {
        ClearAttackMark(volumes[index]);
    }
    volume_count = kept;
}

void Attack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    int move_id,
    AttackMark* volumes,
    int& volume_count,
    int volume_capacity,
    bool buffer_early_press) {
    if (subjects == nullptr || attacker_index < 0 || attacker_index >= subject_count) {
        return;
    }

    Subject& attacker = subjects[attacker_index];
    attacker.attack_cooldown = AdvanceAttackTimer(attacker.attack_cooldown, frame_seconds);
    if (attacker.remaining <= 0) {
        attacker.buffered_move = kMoveNone;
        return;
    }
    // Judged after this frame's tick. A press earlier than the window is not stored.
    if (buffer_early_press && move_id != kMoveNone && attacker.attack_cooldown > 0.0f &&
        attacker.attack_cooldown <= kAttackBufferWindow) {
        attacker.buffered_move = move_id;
    }
    int fired = kMoveNone;
    if (attacker.attack_cooldown <= 0.0f) {
        if (move_id != kMoveNone) {
            fired = move_id;
        } else if (buffer_early_press && attacker.buffered_move != kMoveNone) {
            fired = attacker.buffered_move;
        }
    }
    MoveRow row{};
    if (fired == kMoveNone || !TryMove(fired, row)) {
        return;
    }
    attacker.buffered_move = kMoveNone;
    attacker.attack_cooldown = row.interval;

    if (volumes == nullptr || volume_count < 0 || volume_count >= volume_capacity) {
        return;
    }
    AttackMark& volume = volumes[volume_count];
    ShowAttackMark(volume, AttackBox(attacker, fired), attacker_index);
    volume_count += 1;
}
