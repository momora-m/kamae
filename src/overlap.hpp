#pragma once

struct Subject;

// Axis-aligned box. Walls and subjects use the same shape. Yaw is ignored.
struct AxisBox {
    float min_x;
    float min_y;
    float min_z;
    float max_x;
    float max_y;
    float max_z;
};

constexpr int kWallCount = 4;

// The subject's axis-aligned cube. Yaw is ignored, matching walk and attack.
AxisBox SubjectBox(const Subject& subject);

// True when the volumes intersect. Shared faces alone do not overlap.
bool AxisBoxesOverlap(const AxisBox& a, const AxisBox& b);

// Four immovable boxes. Inner faces sit on x, z = ±floor_half.
void WallBoxes(AxisBox (&walls)[kWallCount], float floor_half);

// After a walk, cancel X or Z when that axis overlaps a wall or another subject
// that still has remaining. previous_x and previous_z are the mover's position
// before this frame's step. Y is left alone. Other subjects are not pushed.
void ResolveHorizontalOverlap(
    Subject* subjects,
    int subject_count,
    int mover_index,
    float previous_x,
    float previous_z,
    float floor_half);
