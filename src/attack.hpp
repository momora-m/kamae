#pragma once

#include "overlap.hpp"

struct AttackMark;
struct Subject;

// Axis-aligned volume in front of the subject's yaw. Off-axis yaw uses the corners' bounds.
AxisBox AttackBox(const Subject& attacker);

// One press from the attacker, after walking. The hit is AttackBox, tested once.
// Other subjects in it lose one remaining. The attacker is unchanged. A subject with
// no remaining does not attack and is not a target. When an attack is issued, mark
// receives that same box for this frame only. attack_pressed is the caller's input.
void Attack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    bool attack_pressed,
    AttackMark* mark);
