#pragma once

struct Subject;

constexpr float kWalkSpeed = 2.5f;
// Pulls the cube down. From a height of 1, landing takes about half a second.
constexpr float kGravity = 10.0f;
// One hop. The center rises by the cube half-extent, which is the smallest
// height that still reads as leaving the floor. Speed is sqrt(2 * gravity * height).
constexpr float kJumpHeight = 0.5f;
// Six vsync frames at speed 10 travel 1.0, the short move's forward reach.
constexpr int kDodgeFrames = 6;
constexpr float kDodgeSpeed = 10.0f;
constexpr float kFloorCenterY = 0.0f;
constexpr float kFloorEpsilon = 0.0001f;

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

// True when the cube center is on or below the floor plane.
bool SubjectOnFloor(const Subject& subject);

// Writes horizontal velocity from the walk intent. Zero intent clears
// horizontal velocity. walk_seconds may be shorter than frame_seconds so an
// approach does not step past the target. Position is integrated later.
void WalkCube(
    Subject& subject,
    const HorizontalBasis& basis,
    const HorizontalWalk& walk,
    float walk_seconds,
    float frame_seconds,
    bool face_move);

// Adds gravity to Y velocity, then position += velocity * frame_seconds.
// Clamps the center to the floor and clears Y velocity when landed.
// frame_seconds 0 leaves position and velocity as they are.
void IntegrateSubject(Subject& subject, float frame_seconds);
