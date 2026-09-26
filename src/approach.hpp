#pragma once

#include "actions.hpp"
#include "walk.hpp"

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

// Facing toward the target on XZ. Used as the walk basis while Approaching.
HorizontalBasis BasisTowardTarget(float dx, float dz, float length);

// Reads the opponent controller. Updates facing and the in-range wait.
// Returns the same actions Walk and Attack already accept. Does not walk
// and does not call Attack. keep_yaw leaves the mover's yaw alone.
SubjectActions OpponentActions(
    Subject* subjects,
    int subject_count,
    int mover_index,
    int target_index,
    float frame_seconds,
    bool keep_yaw);
