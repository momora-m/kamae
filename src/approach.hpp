#pragma once

#include "actions.hpp"
#include "walk.hpp"

struct Subject;

// First opponent states. Each one is a horizontal walk, which move to swing,
// and whether to guard. Backing off after a swing is a backward walk, not
// another state. Circling while approaching is a strafe on that same walk.
// Guarding instead of a swing is still Wait, with guard set.
enum class OpponentState {
    Approach,
    Wait,
    Fire,
};

// What one state delivers for this frame. Not keys.
struct OpponentCommand {
    OpponentState state;
    HorizontalWalk walk;
    int move;
};

// Approach walks forward on the basis toward the target and does not attack.
// Wait and Fire stand still. Fire asks AdvanceAttack to start that swing.
// A negative forward walk is the retreat after that swing. Guard is not a
// new state. On a Fire frame, a player's overlapping poke or long box
// becomes a Wait with guard instead.
OpponentCommand CommandForOpponentState(OpponentState state);

// Facing toward the target on XZ. Used as the walk basis while Approaching.
HorizontalBasis BasisTowardTarget(float dx, float dz, float length);

// Reads the opponent controller. Updates facing and the in-range wait.
// Returns the same actions walk and AdvanceAttack already accept. Does not
// walk and does not start a swing. keep_yaw leaves the mover's yaw alone.
SubjectActions OpponentActions(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    bool keep_yaw);
