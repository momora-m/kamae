#pragma once

#include "overlap.hpp"

struct AttackMark;
struct Subject;

// Player repeat. Opponent repeat is longer. The opponent's first swing also waits
// kOpponentReactionDelay after the box overlaps; that wait lives in the approach.
constexpr float kPlayerAttackInterval = 0.4f;
constexpr float kOpponentAttackInterval = 0.8f;
constexpr float kAttackBufferWindow = 0.15f;
constexpr float kOpponentReactionDelay = 0.5f;
// Below zero: the opponent is not standing in range, so the next entry waits again.
constexpr float kAttackReactionIdle = -1.0f;
// After a hit, the next this many trial frames pass 0 seconds into WalkCube and Attack.
// Not a global pause, and not the attack volume lifetime.
constexpr int kHitstopFrames = 4;
// A leftover under a tenth of a millisecond is zero. A 0.1 second frame must not
// stretch 0.4, 0.5, or 0.8 by another frame.
constexpr float kAttackTimerEpsilon = 0.0001f;

inline float AdvanceAttackTimer(float remaining, float frame_seconds) {
    if (frame_seconds > 0.0f) {
        remaining -= frame_seconds;
    }
    if (remaining < kAttackTimerEpsilon) {
        return 0.0f;
    }
    return remaining;
}

// Axis-aligned volume in front of the subject's yaw. Off-axis yaw uses the corners' bounds.
AxisBox AttackBox(const Subject& attacker);

// One press from the attacker, after walking. The hit is AttackBox.
// Other subjects in it lose one remaining. The attacker is unchanged. A subject with
// no remaining does not attack and is not a target. When an attack is issued, the
// same box is appended to the trial volume list for kAttackVolumeActiveFrames.
// Later frames keep the box only through TickAttackVolumes. A subject already hit
// by this volume is not hit again. The list is not a per-subject slot. A full list
// does not spawn. attack_pressed is the caller's input.
// attack_interval is stored on the attacker when a swing actually fires.
// buffer_early_press remembers a player Space only in the last kAttackBufferWindow
// of cooldown and fires it once when cooldown reaches 0. Earlier presses are dropped.
// That memory is not a windup. Opponents pass false.
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
    bool buffer_early_press);

// One frame of a volume Attack already spawned. Shortens remaining_frames.
// While frames remain, the stored box is tested again. Walk and Δt are not used.
void TickAttackVolume(AttackMark& mark, Subject* subjects, int subject_count);

// One frame of every volume the trial owns. Expired entries leave the list.
void TickAttackVolumes(AttackMark* volumes, int& volume_count, Subject* subjects, int subject_count);
