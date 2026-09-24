#pragma once

struct AttackMark;
struct Subject;

// Walk toward the target on XZ at the player's speed. Inside the shared attack
// volume, stop, face the target, and attack on the same interval. Stops when
// either subject has no remaining. Contact does not reduce remaining.
// keep_yaw leaves the mover's yaw alone for this frame.
void ApproachAndAttack(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    AttackMark* mark,
    bool keep_yaw);
