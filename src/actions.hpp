#pragma once

#include "walk.hpp"

// Horizontal walk and attack for one frame. Either controller emits this.
struct SubjectActions {
    HorizontalWalk walk;
    bool attack;
};

// The only place that reads keys. Builds the player's walk and attack.
// No viewport hover, or text input owns the keys: nothing reaches the player.
SubjectActions PlayerActionsFromKeys(bool viewport_hovered);
