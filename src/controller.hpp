#pragma once

struct SceneState;

// Two controllers. Both emit the same horizontal walk and move id.
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

// Yaw used to draw and to walk. Editing keeps the stored orbit.
// Play sits behind camera_follow_yaw, plus a drag offset that stays after
// the button is released. Releasing move keys does not copy the body's yaw
// onto that follow yaw. A stopped trial sits behind the player's yaw, plus
// the same offset.
float ActiveCameraYaw(const SceneState& scene);
float ActiveCameraPitch(const SceneState& scene);

// Starts the player's dodge before hit checks when E is pressed on the floor.
// Sets horizontal velocity once. Does nothing in the air, during hitstop,
// or while a dodge is already running. Opponents do not call this.
void TryStartPlayerDodge(SceneState& scene, float frame_seconds, bool viewport_hovered);

// One attached controller. Emits actions, then the trial walks and attacks.
// remaining 0 does nothing. Walk and Attack do not treat index 0 as special.
void StepController(
    SceneState& scene,
    const Controller& controller,
    float frame_seconds,
    bool viewport_hovered,
    bool keep_yaw);
