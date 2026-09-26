#pragma once

#include "walk.hpp"

struct AttackMark;
struct Subject;

// First opponent states. Each one is a horizontal walk plus attack yes/no.
// Circling and backing off would only change the walk direction.
enum class OpponentState {
    Approach,
    Wait,
    Fire,
};

// What one state delivers for this frame. Not keys.
struct OpponentCommand {
    OpponentState state;
    HorizontalWalk walk;
    bool attack;
};

// Approach walks forward on the basis toward the target and does not attack.
// Wait and Fire stand still. Fire asks the same Attack to swing.
OpponentCommand CommandForOpponentState(OpponentState state);

// Walk toward the target on XZ at the player's speed while Approaching.
// Inside the attack volume, Wait or Fire: stop and face the target.
// The first swing waits kOpponentReactionDelay after the box overlaps.
// Later swings, while still in range, use kOpponentAttackInterval inside Attack.
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
