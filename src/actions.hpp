#pragma once

#include "walk.hpp"

// The player's horizontal walk and attack for one frame.
struct PlayerActions {
    HorizontalWalk walk;
    bool attack;
};

// The only place that reads keys. Builds the player's walk and attack.
// No viewport hover, or text input owns the keys: nothing reaches the player.
PlayerActions PlayerActionsFromKeys(bool viewport_hovered);
