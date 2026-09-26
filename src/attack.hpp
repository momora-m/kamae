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

// One press from the attacker, after walking. The hit is AttackBox, tested once.
// Other subjects in it lose one remaining. The attacker is unchanged. A subject with
// no remaining does not attack and is not a target. When an attack is issued, mark
// receives that same box for this frame only. attack_pressed is the caller's input.
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
    AttackMark* mark,
    float attack_interval,
    bool buffer_early_press);
