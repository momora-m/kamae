#include "overlap.hpp"

#include "renderer.hpp"

namespace {

constexpr float kWallThickness = 1.0f;
constexpr float kWallHeight = 2.0f;

bool RangesOverlap(float min_a, float max_a, float min_b, float max_b) {
    return min_a < max_b && max_a > min_b;
}

AxisBox BoxFromCenter(float x, float y, float z, float half) {
    return AxisBox{
        x - half,
        y - half,
        z - half,
        x + half,
        y + half,
        z + half,
    };
}

bool Blocked(const Subject& mover, const Subject* subjects, int subject_count, int mover_index) {
    const AxisBox mover_box = SubjectBox(mover);

    AxisBox walls[kWallCount];
    WallBoxes(walls);
    for (const AxisBox& wall : walls) {
        if (AxisBoxesOverlap(mover_box, wall)) {
            return true;
        }
    }

    for (int index = 0; index < subject_count; ++index) {
        if (index == mover_index) {
            continue;
        }
        const Subject& other = subjects[index];
        if (other.remaining <= 0) {
            continue;
        }
        const AxisBox other_box = SubjectBox(other);
        if (AxisBoxesOverlap(mover_box, other_box)) {
            return true;
        }
    }
    return false;
}

}  // namespace

AxisBox SubjectBox(const Subject& subject) {
    return BoxFromCenter(subject.position[0], subject.position[1], subject.position[2], kCubeHalfExtent);
}

bool AxisBoxesOverlap(const AxisBox& a, const AxisBox& b) {
    return RangesOverlap(a.min_x, a.max_x, b.min_x, b.max_x) &&
           RangesOverlap(a.min_y, a.max_y, b.min_y, b.max_y) &&
           RangesOverlap(a.min_z, a.max_z, b.min_z, b.max_z);
}

void WallBoxes(AxisBox (&walls)[kWallCount]) {
    const float inner = kFloorHalfExtent;
    const float outer = kFloorHalfExtent + kWallThickness;
    const float bottom = -kCubeHalfExtent;
    const float top = bottom + kWallHeight;
    // ±X walls own the corner posts. ±Z walls stop on the inner faces, so the
    // visible end caps are back faces and do not z-fight the posts.
    walls[0] = AxisBox{inner, bottom, -outer, outer, top, outer};
    walls[1] = AxisBox{-outer, bottom, -outer, -inner, top, outer};
    walls[2] = AxisBox{-inner, bottom, inner, inner, top, outer};
    walls[3] = AxisBox{-inner, bottom, -outer, inner, top, -inner};
}

void ResolveHorizontalOverlap(
    Subject* subjects,
    int subject_count,
    int mover_index,
    float previous_x,
    float previous_z) {
    if (subjects == nullptr || mover_index < 0 || mover_index >= subject_count) {
        return;
    }

    Subject& mover = subjects[mover_index];
    const float moved_z = mover.position[2];

    mover.position[2] = previous_z;
    if (Blocked(mover, subjects, subject_count, mover_index)) {
        mover.position[0] = previous_x;
    }

    mover.position[2] = moved_z;
    if (Blocked(mover, subjects, subject_count, mover_index)) {
        mover.position[2] = previous_z;
    }
}
