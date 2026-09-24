#pragma once

struct SceneState;

// Horizontal move axes on the XZ plane. WalkCube does not know where they came from.
struct HorizontalBasis {
    float forward_x;
    float forward_z;
    float right_x;
    float right_z;
};

void WalkCube(
    SceneState& scene,
    const HorizontalBasis& basis,
    float frame_seconds,
    bool viewport_hovered,
    bool face_move);
