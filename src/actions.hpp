#pragma once

#include "walk.hpp"

// No swing. Not a row in the move table.
constexpr int kMoveNone = 0;
// First row. The current poke: forward 1.0, lateral 0.5, interval 0.4 seconds.
constexpr int kMovePoke = 1;
// Longer reach and a longer interval than the poke. See ADR 0027.
constexpr int kMoveLong = 2;

// Horizontal walk, which move to swing, whether to guard, jump, or dodge,
// for one frame. Either controller emits this. kMoveNone does not swing.
// Guard, jump, and dodge are not moves.
struct SubjectActions {
    HorizontalWalk walk;
    int move = kMoveNone;
    bool guard = false;
    bool jump = false;
    bool dodge = false;
};

// The only place that reads keys. Builds the player's walk, attack, and guard.
// Shift holds a guard. No viewport hover, or text input owns the keys: nothing
// reaches the player.
SubjectActions PlayerActionsFromKeys(bool viewport_hovered);
