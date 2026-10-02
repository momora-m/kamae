#pragma once

#include "overlap.hpp"

// Draw and hit test share these frames. The character's startup and recovery
// live on the move row, not on the volume. See ADR 0030.
constexpr int kAttackVolumeActiveFrames = 3;

// The hit box for one swing. SpawnAttackVolume writes it into the trial list.
// Each later frame shortens remaining_frames. While frames remain, the box
// stays where it was spawned. hit_mask records subjects already damaged by
// this volume, so one swing reduces a subject once. attacker_index is who
// spawned it.
struct AttackMark {
    bool visible = false;
    int remaining_frames = 0;
    unsigned hit_mask = 0;
    int attacker_index = -1;
    AxisBox box{};
    float color[3] = {0.93f, 0.82f, 0.28f};
    // Attacker's yaw forward when the volume was spawned. Yaw 0 faces +Z.
    float forward_x = 0.0f;
    float forward_z = 1.0f;
    // Devour volumes restore one round on a connected overlap. They do not
    // reduce remaining. Other volumes leave this false.
    bool restores_round = false;
};

void ClearAttackMark(AttackMark& mark);
void ShowAttackMark(AttackMark& mark, const AxisBox& box, int attacker_index);
void ClearAttackVolumes(AttackMark* volumes, int& volume_count);
