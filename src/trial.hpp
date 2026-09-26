#pragma once

#include "actions.hpp"
#include "walk.hpp"

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

// Puts the remembered layout back and starts play. Remaining is 3 and cooldown
// is 0. Does not remember the stopped layout. Does not return to editing.
void RestartTrial(SceneState& scene);

// Why a trial stops. Continue leaves the session in play.
// Zero opponents never produce OpponentsDepleted.
// Both depleted on the same frame is PlayerDepleted.
enum class TrialStop {
    Continue,
    PlayerDepleted,
    OpponentsDepleted,
};

TrialStop TrialStopReason(const SceneState& scene);

// Walk and attack from emitted actions. Overlap is resolved after the step.
// walk_seconds may be shorter than frame_seconds so an approach does not
// step past the target. Walk and Attack do not treat index 0 as special.
// Collect overlaps from the trial volumes and apply remaining and hitstop.
// Volumes do not change remaining themselves.
void ApplyVolumeHits(SceneState& scene);

void ApplySubjectActions(
    SceneState& scene,
    int subject_index,
    const SubjectActions& actions,
    const HorizontalBasis& basis,
    bool face_move,
    float walk_seconds,
    float frame_seconds,
    bool buffer_early_press);
