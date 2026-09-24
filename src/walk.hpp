#pragma once

struct Subject;

// Horizontal move axes on the XZ plane. WalkCube does not know where they came from.
struct HorizontalBasis {
    float forward_x;
    float forward_z;
    float right_x;
    float right_z;
};

// Moves one subject on XZ. Overlap with walls and other subjects is a later step.
void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    float frame_seconds,
    bool viewport_hovered,
    bool face_move);
