#include "controller.hpp"

#include "actions.hpp"
#include "approach.hpp"
#include "attack.hpp"
#include "renderer.hpp"
#include "trial.hpp"
#include "walk.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr float kDirectionEpsilon = 0.001f;

// Camera yaw 0 looks toward -Z. Pitch is ignored so the move stays on the ground.
// The eye sits at (sin(yaw), cos(yaw)) on XZ, so yaw 0 is on +Z looking toward -Z.
HorizontalBasis BasisFromCameraYaw(float yaw) {
    return HorizontalBasis{
        -std::sin(yaw),
        -std::cos(yaw),
        -std::cos(yaw),
        std::sin(yaw),
    };
}

// Cube yaw 0 faces +Z. Walking reads this yaw, not the camera.
HorizontalBasis BasisFromCubeYaw(float yaw_radians) {
    const float s = std::sin(yaw_radians);
    const float c = std::cos(yaw_radians);
    return HorizontalBasis{
        s,
        c,
        c,
        -s,
    };
}

float ApproachWalkSeconds(const Subject& mover, const Subject& target, float frame_seconds) {
    const float dx = target.position[0] - mover.position[0];
    const float dz = target.position[2] - mover.position[2];
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < kDirectionEpsilon || frame_seconds <= 0.0f) {
        return 0.0f;
    }
    const float step = kWalkSpeed * frame_seconds;
    if (step > length) {
        return length / kWalkSpeed;
    }
    return frame_seconds;
}

HorizontalBasis ApproachBasis(const Subject& mover, const Subject& target) {
    const float dx = target.position[0] - mover.position[0];
    const float dz = target.position[2] - mover.position[2];
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < kDirectionEpsilon) {
        return BasisFromCubeYaw(mover.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f));
    }
    return BasisTowardTarget(dx, dz, length);
}

}  // namespace

void HoldPlayCameraFollow(SceneState& scene, bool viewport_hovered) {
    if (scene.session != SessionMode::Trial || scene.subject_count <= kPlayer) {
        return;
    }
    const Subject& player = scene.subjects[kPlayer];
    const SubjectActions actions = PlayerActionsFromKeys(viewport_hovered);
    const float input = std::sqrt(
        actions.walk.strafe * actions.walk.strafe + actions.walk.forward * actions.walk.forward);
    // A held direction must keep the basis. Copying the facing this move writes
    // turns A, S, and D onto a new camera every frame. W stays put only because
    // it already faces the look direction.
    if (player.remaining <= 0 || input < kDirectionEpsilon) {
        scene.camera_follow_yaw = player.rotation_degrees[1];
    }
}

float ActiveCameraYaw(const SceneState& scene) {
    if (scene.session == SessionMode::Editing) {
        return scene.camera_yaw;
    }
    // Yaw 0 faces +Z. Half a turn puts the eye on the back, looking along that facing.
    // Trial uses the held follow yaw. Stopped has no walk, so the body yaw is the back.
    const float facing_degrees = scene.session == SessionMode::Trial
        ? scene.camera_follow_yaw
        : scene.subjects[kPlayer].rotation_degrees[1];
    const float facing = facing_degrees * (std::numbers::pi_v<float> / 180.0f);
    return facing + std::numbers::pi_v<float> + scene.camera_yaw_offset;
}

void TryStartPlayerDodge(SceneState& scene, float frame_seconds, bool viewport_hovered) {
    if (frame_seconds <= 0.0f || scene.subject_count <= kPlayer) {
        return;
    }
    Subject& player = scene.subjects[kPlayer];
    if (player.remaining <= 0 || player.dodge_frames > 0 || !SubjectOnFloor(player)) {
        return;
    }
    const SubjectActions actions = PlayerActionsFromKeys(viewport_hovered);
    if (!actions.dodge) {
        return;
    }

    const float input = std::sqrt(actions.walk.strafe * actions.walk.strafe + actions.walk.forward * actions.walk.forward);
    float dir_x = 0.0f;
    float dir_z = 1.0f;
    if (input > kDirectionEpsilon) {
        const float strafe = actions.walk.strafe / input;
        const float forward = actions.walk.forward / input;
        const HorizontalBasis basis = BasisFromCameraYaw(ActiveCameraYaw(scene));
        dir_x = basis.right_x * strafe + basis.forward_x * forward;
        dir_z = basis.right_z * strafe + basis.forward_z * forward;
    } else {
        const float yaw = player.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
        dir_x = std::sin(yaw);
        dir_z = std::cos(yaw);
    }
    player.velocity[0] = dir_x * kDodgeSpeed;
    player.velocity[2] = dir_z * kDodgeSpeed;
    player.dodge_frames = kDodgeFrames;
}

float ActiveCameraPitch(const SceneState& scene) {
    const float pitch =
        scene.session == SessionMode::Editing ? scene.camera_pitch : scene.camera_pitch + scene.camera_pitch_offset;
    return std::clamp(pitch, -kCameraPitchLimit, kCameraPitchLimit);
}

void AttachControllers(SceneState& scene) {
    for (int index = 0; index < kSubjectCapacity; ++index) {
        if (index == 0) {
            scene.controllers[index] = Controller{ControllerKind::PlayerKeys, 0, 0};
            continue;
        }
        scene.controllers[index] = Controller{ControllerKind::OpponentApproach, index, 0};
    }
}

void StepController(
    SceneState& scene,
    const Controller& controller,
    float frame_seconds,
    bool viewport_hovered,
    bool keep_yaw) {
    const int subject_index = controller.subject_index;
    if (subject_index < 0 || subject_index >= scene.subject_count) {
        return;
    }
    if (scene.subjects[subject_index].remaining <= 0) {
        return;
    }

    if (controller.kind == ControllerKind::PlayerKeys) {
        const HorizontalBasis basis = BasisFromCameraYaw(ActiveCameraYaw(scene));
        const SubjectActions actions = PlayerActionsFromKeys(viewport_hovered);
        ApplySubjectActions(
            scene,
            subject_index,
            actions,
            basis,
            true,
            frame_seconds,
            frame_seconds,
            true);
        return;
    }

    const int target_index = controller.target_index;
    const SubjectActions actions = OpponentActions(
        scene.subjects,
        scene.subject_count,
        subject_index,
        target_index,
        frame_seconds,
        keep_yaw);
    HorizontalBasis basis = BasisFromCubeYaw(
        scene.subjects[subject_index].rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f));
    const bool backing_up = actions.walk.forward < 0.0f;
    float walk_seconds = 0.0f;
    if (target_index >= 0 && target_index < scene.subject_count) {
        basis = ApproachBasis(scene.subjects[subject_index], scene.subjects[target_index]);
        if (backing_up || actions.walk.strafe != 0.0f) {
            walk_seconds = frame_seconds;
        } else if (actions.walk.forward > 0.0f) {
            walk_seconds =
                ApproachWalkSeconds(scene.subjects[subject_index], scene.subjects[target_index], frame_seconds);
        }
    }
    ApplySubjectActions(
        scene,
        subject_index,
        actions,
        basis,
        backing_up ? false : !keep_yaw,
        walk_seconds,
        frame_seconds,
        false);
}
