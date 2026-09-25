#pragma once

#include "overlap.hpp"

// One attack frame's hit box. Cleared on the next attack step, so it is not drawn again.
struct AttackMark {
    bool visible = false;
    AxisBox box{};
    float color[3] = {0.93f, 0.82f, 0.28f};
};

void ClearAttackMark(AttackMark& mark);
void ShowAttackMark(AttackMark& mark, const AxisBox& box);
