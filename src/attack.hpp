#pragma once

struct AttackMark;
struct Subject;

// One Space press from the attacker, after walking. The hit is an axis-aligned box
// in front of that subject's yaw, tested once. Other subjects in it lose one remaining.
// The attacker is unchanged. A subject with no remaining is not a target.
// When an attack is issued, mark receives that same box for this frame only.
void Attack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    bool viewport_hovered,
    AttackMark* mark);
