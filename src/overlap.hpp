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

// Four immovable boxes. Inner faces sit on x, z = ±kFloorHalfExtent.
void WallBoxes(AxisBox (&walls)[kWallCount]);

// After a walk, cancel X or Z when that axis overlaps a wall or another subject
// that still has remaining. previous_x and previous_z are the mover's position
// before this frame's step. Y is left alone. Other subjects are not pushed.
void ResolveHorizontalOverlap(
    Subject* subjects,
    int subject_count,
    int mover_index,
    float previous_x,
    float previous_z);
