#pragma once

struct SceneState;

// Remembers the edited layout, sets remaining to 3 and cooldown to 0, then
// starts a trial. A buffered Space and an opponent's in-range wait are cleared.
// Combat progress is not stored. Unsaved edits are included.
void BeginTrial(SceneState& scene);

// Puts the remembered layout back and returns to editing. Does not start again.
void RestoreStartLayout(SceneState& scene);

// True when the player has no remaining, or every opponent in use has none.
// Zero opponents do not end the trial by this check.
bool TrialShouldStop(const SceneState& scene);
