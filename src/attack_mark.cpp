#include "attack_mark.hpp"

namespace {

// Distinct from the player, the opponent, and the walls. One albedo, no per-face colors.
constexpr float kAttackMarkRed = 0.93f;
constexpr float kAttackMarkGreen = 0.82f;
constexpr float kAttackMarkBlue = 0.28f;

}  // namespace

void ClearAttackMark(AttackMark& mark) {
    mark.visible = false;
}

void ShowAttackMark(AttackMark& mark, const AxisBox& box) {
    mark.visible = true;
    mark.box = box;
    mark.color[0] = kAttackMarkRed;
    mark.color[1] = kAttackMarkGreen;
    mark.color[2] = kAttackMarkBlue;
}
