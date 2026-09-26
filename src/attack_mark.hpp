#pragma once

#include "overlap.hpp"

// Startup and recovery are 0, so the whole lifetime is the active range.
// Draw and hit test share these frames. This is not a windup.
constexpr int kAttackVolumeStartupFrames = 0;
constexpr int kAttackVolumeActiveFrames = 3;
constexpr int kAttackVolumeRecoveryFrames = 0;
constexpr int kAttackVolumeLifetimeFrames =
    kAttackVolumeStartupFrames + kAttackVolumeActiveFrames + kAttackVolumeRecoveryFrames;

// The hit box for one swing. Attack spawns it. Each later frame shortens remaining_frames.
// While frames remain, the box stays where it was spawned. hit_mask records subjects
// already damaged by this volume, so one swing reduces a subject once.
struct AttackMark {
    bool visible = false;
    int remaining_frames = 0;
    unsigned hit_mask = 0;
    AxisBox box{};
    float color[3] = {0.93f, 0.82f, 0.28f};
};

void ClearAttackMark(AttackMark& mark);
void ShowAttackMark(AttackMark& mark, const AxisBox& box);
