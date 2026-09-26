#pragma once

struct SceneState;

// Two controllers. Both emit the same horizontal walk and attack yes/no.
enum class ControllerKind {
    PlayerKeys,
    OpponentApproach,
};

// Attached to one subject. PlayerKeys reads keys. OpponentApproach walks
// toward target_index and may ask to attack. Walk and Attack do not see this.
struct Controller {
    ControllerKind kind = ControllerKind::PlayerKeys;
    int subject_index = 0;
    int target_index = 0;
};

// First subject gets PlayerKeys. The rest get OpponentApproach toward subject 0.
void AttachControllers(SceneState& scene);

// One attached controller. Emits actions, then the trial walks and attacks.
// remaining 0 does nothing. Walk and Attack do not treat index 0 as special.
void StepController(
    SceneState& scene,
    const Controller& controller,
    float frame_seconds,
    bool viewport_hovered,
    bool keep_yaw);
