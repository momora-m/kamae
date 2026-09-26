#include "attack.hpp"

#include "attack_mark.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr float kAttackForward = 1.0f;
constexpr float kAttackLateral = 0.5f;

static_assert(kSubjectCapacity <= 32, "the volume hit mask has one bit per subject");

void ApplyVolumeHit(AttackMark& mark, Subject* subjects, int subject_count, int attacker_index) {
    if (subjects == nullptr || subject_count <= 0) {
        return;
    }
    const int count = subject_count < kSubjectCapacity ? subject_count : kSubjectCapacity;
    for (int index = 0; index < count; ++index) {
        if (index == attacker_index) {
            continue;
        }
        Subject& other = subjects[index];
        if (other.remaining <= 0) {
            continue;
        }
        const unsigned bit = 1u << static_cast<unsigned>(index);
        if ((mark.hit_mask & bit) != 0u) {
            continue;
        }
        if (!AxisBoxesOverlap(mark.box, SubjectBox(other))) {
            continue;
        }
        other.remaining -= 1;
        other.hitstop_frames = kHitstopFrames;
        mark.hit_mask |= bit;
    }
}

}  // namespace

static_assert(kAttackReactionIdle < 0.0f, "an idle reaction is not a finished wait");
static_assert(
    Subject{}.attack_reaction == kAttackReactionIdle, "a new subject has not started the in-range wait");

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

void TickAttackVolume(AttackMark& mark, Subject* subjects, int subject_count) {
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
    ApplyVolumeHit(mark, subjects, subject_count, mark.attacker_index);
}

void TickAttackVolumes(AttackMark* volumes, int& volume_count, Subject* subjects, int subject_count) {
    if (volumes == nullptr || volume_count <= 0) {
        volume_count = 0;
        return;
    }
    int kept = 0;
    for (int index = 0; index < volume_count; ++index) {
        TickAttackVolume(volumes[index], subjects, subject_count);
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
    bool attack_pressed,
    AttackMark* volumes,
    int& volume_count,
    int volume_capacity,
    float attack_interval,
    bool buffer_early_press) {
    if (subjects == nullptr || attacker_index < 0 || attacker_index >= subject_count) {
        return;
    }

    Subject& attacker = subjects[attacker_index];
    attacker.attack_cooldown = AdvanceAttackTimer(attacker.attack_cooldown, frame_seconds);
    if (attacker.remaining <= 0) {
        attacker.attack_buffered = false;
        return;
    }
    // Judged after this frame's tick. A press earlier than the window is not stored.
    if (buffer_early_press && attack_pressed && attacker.attack_cooldown > 0.0f &&
        attacker.attack_cooldown <= kAttackBufferWindow) {
        attacker.attack_buffered = true;
    }
    const bool buffered = buffer_early_press && attacker.attack_buffered;
    if (attacker.attack_cooldown > 0.0f || (!attack_pressed && !buffered)) {
        return;
    }
    attacker.attack_buffered = false;
    attacker.attack_cooldown = attack_interval;

    if (volumes == nullptr || volume_count < 0 || volume_count >= volume_capacity) {
        return;
    }
    AttackMark& volume = volumes[volume_count];
    ShowAttackMark(volume, AttackBox(attacker), attacker_index);
    volume_count += 1;
    ApplyVolumeHit(volume, subjects, subject_count, attacker_index);
}
