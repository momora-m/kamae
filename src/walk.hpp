#pragma once

struct Subject;

// Horizontal move axes on the XZ plane. WalkCube does not know where they came from.
struct HorizontalBasis {
    float forward_x;
    float forward_z;
    float right_x;
    float right_z;
};

// Moves one subject. The caller passes the player, so the floor edge applies only to them.
void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    float frame_seconds,
    bool viewport_hovered,
    bool face_move);
