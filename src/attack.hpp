#pragma once

#include "overlap.hpp"

struct AttackMark;
struct Subject;

// Time until the same swing can fire again. The opponent does not stay and repeat
// on a longer interval. After a swing they walk back, then wait
// kOpponentReactionDelay once the box overlaps again. That wait lives in the approach.
constexpr float kPlayerAttackInterval = 0.4f;
// Second move. Farther than the poke, and longer before it can fire again.
// At the builtin spacing of 4, this box still does not overlap.
constexpr float kLongMoveForward = 2.0f;
constexpr float kLongMoveInterval = 1.0f;
constexpr float kAttackBufferWindow = 0.15f;
constexpr float kOpponentReactionDelay = 0.5f;
// Below zero: the opponent is not standing in range, so the next entry waits again.
constexpr float kAttackReactionIdle = -1.0f;
// After a hit, the next this many trial frames pass 0 seconds into WalkCube and Attack.
// Not a global pause, and not the attack volume lifetime.
constexpr int kHitstopFrames = 4;
// A leftover under a tenth of a millisecond is zero. A 0.1 second frame must not
// stretch 0.4, 0.5, or 1.0 by another frame.
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

// Reach and interval for one move id. kMoveNone is not a row.
struct MoveRow {
    float forward = 1.0f;
    float lateral = 0.5f;
    float interval = 0.4f;
};

// False when move_id is kMoveNone or unknown. Row 0 is the poke. Row 1 is the long move.
bool TryMove(int move_id, MoveRow& row);

// Axis-aligned volume in front of the subject's yaw, using that move's reach.
// Off-axis yaw uses the corners' bounds. An unknown move yields no extension.
AxisBox AttackBox(const Subject& attacker, int move_id);

// One move from the attacker, after walking. Spawns that row's box into the trial
// list for kAttackVolumeActiveFrames. Does not apply remaining or hitstop.
// A subject with no remaining does not attack. Later frames keep the box only
// through TickAttackVolumes. The list is not a per-subject slot. A full list
// does not spawn. move_id is the caller's input. kMoveNone does not swing.
// The row's interval is stored on the attacker when a swing actually fires.
// buffer_early_press remembers the player's last move only in the last
// kAttackBufferWindow of cooldown and fires it once when cooldown reaches 0.
// Earlier presses are dropped. That memory is not a windup. Opponents pass false.
void Attack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    int move_id,
    AttackMark* volumes,
    int& volume_count,
    int volume_capacity,
    bool buffer_early_press);

// One frame of a volume Attack already spawned. Shortens remaining_frames.
// Overlap is not tested here. Walk and Δt are not used.
void TickAttackVolume(AttackMark& mark);

// One frame of every volume the trial owns. Expired entries leave the list.
void TickAttackVolumes(AttackMark* volumes, int& volume_count);
