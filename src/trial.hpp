#pragma once

struct SceneState;
struct Subject;

// This trial frame's seconds for one subject. The frame default is frame_seconds.
// A subject that was hit gets 0 for kHitstopFrames, starting the frame after the hit.
// Calling this counts one of those frames down. Keys are not read here.
float TakeSubjectFrameSeconds(Subject& subject, float frame_seconds);

// Remembers the edited layout, sets remaining to 3 and cooldown to 0, then
// starts a trial. A buffered Space, an opponent's in-range wait, and hitstop
// frames are cleared. Combat progress is not stored. Unsaved edits are included.
void BeginTrial(SceneState& scene);

// Puts the remembered layout back and returns to editing. Does not start again.
void RestoreStartLayout(SceneState& scene);

// True when the player has no remaining, or every opponent in use has none.
// Zero opponents do not end the trial by this check.
bool TrialShouldStop(const SceneState& scene);
