#pragma once

struct Subject;

constexpr float kWalkSpeed = 2.5f;

// Horizontal move axes on the XZ plane. WalkCube does not know where they came from.
struct HorizontalBasis {
    float forward_x;
    float forward_z;
    float right_x;
    float right_z;
};

// Strafe and forward on that basis. Not keys. A zero intent does not move.
struct HorizontalWalk {
    float strafe;
    float forward;
};

// Moves one subject on XZ from the walk intent. Overlap is a later step.
void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    const HorizontalWalk& walk,
    float frame_seconds,
    bool face_move);
