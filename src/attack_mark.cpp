#include "attack_mark.hpp"

namespace {

// Distinct from the player, the opponent, and the walls. One albedo, no per-face colors.
constexpr float kAttackMarkRed = 0.93f;
constexpr float kAttackMarkGreen = 0.82f;
constexpr float kAttackMarkBlue = 0.28f;

}  // namespace

static_assert(kAttackVolumeStartupFrames == 0, "the first ship has no windup");
static_assert(kAttackVolumeRecoveryFrames == 0, "the first ship has no recovery after the hit");
static_assert(
    kAttackVolumeLifetimeFrames == kAttackVolumeActiveFrames,
    "with startup and recovery at 0, the lifetime is the active range");
static_assert(kAttackVolumeActiveFrames == 3, "draw and hit last three frames");

void ClearAttackMark(AttackMark& mark) {
    mark.visible = false;
    mark.remaining_frames = 0;
    mark.hit_mask = 0;
    mark.attacker_index = -1;
}

void ShowAttackMark(AttackMark& mark, const AxisBox& box, int attacker_index) {
    mark.visible = true;
    mark.remaining_frames = kAttackVolumeActiveFrames;
    mark.hit_mask = 0;
    mark.attacker_index = attacker_index;
    mark.box = box;
    mark.color[0] = kAttackMarkRed;
    mark.color[1] = kAttackMarkGreen;
    mark.color[2] = kAttackMarkBlue;
}

void ClearAttackVolumes(AttackMark* volumes, int& volume_count) {
    if (volumes == nullptr) {
        volume_count = 0;
        return;
    }
    for (int index = 0; index < volume_count; ++index) {
        ClearAttackMark(volumes[index]);
    }
    volume_count = 0;
}
