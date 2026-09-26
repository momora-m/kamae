#pragma once

struct AttackMark;
struct Subject;

// Walk toward the target on XZ at the player's speed. Inside the attack volume,
// stop and face the target. The first swing waits kOpponentReactionDelay after
// the box overlaps. Later swings, while still in range, use kOpponentAttackInterval.
// Leaving range clears the wait. Stops when either subject has no remaining.
// Contact does not reduce remaining. keep_yaw leaves the mover's yaw alone for this frame.
void ApproachAndAttack(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    AttackMark* mark,
    bool keep_yaw,
    float floor_half);
