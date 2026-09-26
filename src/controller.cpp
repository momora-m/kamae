#include "controller.hpp"

#include "actions.hpp"
#include "approach.hpp"
#include "attack.hpp"
#include "renderer.hpp"
#include "trial.hpp"
#include "walk.hpp"

#include <cmath>
#include <numbers>

namespace {

constexpr float kDirectionEpsilon = 0.001f;

// Camera yaw 0 looks toward -Z. Pitch is ignored so the move stays on the ground.
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
        const float player_yaw =
            scene.subjects[subject_index].rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
        const HorizontalBasis basis = scene.walk_with_camera_yaw ? BasisFromCameraYaw(scene.camera_yaw)
                                                                 : BasisFromCubeYaw(player_yaw);
        const SubjectActions actions = PlayerActionsFromKeys(viewport_hovered);
        ApplySubjectActions(
            scene,
            subject_index,
            actions,
            basis,
            scene.walk_with_camera_yaw,
            frame_seconds,
            frame_seconds,
            kPlayerAttackInterval,
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
    float walk_seconds = 0.0f;
    if (target_index >= 0 && target_index < scene.subject_count) {
        basis = ApproachBasis(scene.subjects[subject_index], scene.subjects[target_index]);
        walk_seconds =
            ApproachWalkSeconds(scene.subjects[subject_index], scene.subjects[target_index], frame_seconds);
    }
    ApplySubjectActions(
        scene,
        subject_index,
        actions,
        basis,
        !keep_yaw,
        walk_seconds,
        frame_seconds,
        kOpponentAttackInterval,
        false);
}
