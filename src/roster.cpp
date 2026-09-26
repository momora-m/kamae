#include "roster.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "controller.hpp"
#include "renderer.hpp"

namespace {

constexpr float kSpawnX = 0.0f;
constexpr float kSpawnY = 0.0f;
constexpr float kSpawnZ = 4.0f;
constexpr float kSpawnYaw = 180.0f;
constexpr float kSpawnRed = 0.25f;
constexpr float kSpawnGreen = 0.42f;
constexpr float kSpawnBlue = 0.68f;

}  // namespace

bool CanAddSubject(const SceneState& scene) {
    return scene.subject_count < kSubjectCapacity;
}

bool CanRemoveSubject(const SceneState& scene) {
    return scene.subject_count > kPlayer + 1;
}

void AddSubject(SceneState& scene) {
    if (!CanAddSubject(scene)) {
        return;
    }
    Subject& subject = scene.subjects[scene.subject_count];
    subject = Subject{
        {kSpawnX, kSpawnY, kSpawnZ},
        {0.0f, kSpawnYaw, 0.0f},
        {kSpawnRed, kSpawnGreen, kSpawnBlue},
        3,
        0.0f,
    };
    scene.subject_count += 1;
    AttachControllers(scene);
}

void ApplyBuiltinLayout(SceneState& scene) {
    const SceneState builtin;
    scene.floor_half = builtin.floor_half;
    scene.subject_count = builtin.subject_count;
    for (int index = 0; index < builtin.subject_count && index < kSubjectCapacity; ++index) {
        scene.subjects[index] = builtin.subjects[index];
    }
    for (int index = scene.subject_count; index < kSubjectCapacity; ++index) {
        scene.subjects[index].remaining = 0;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].attack_buffered = false;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        scene.subjects[index].hitstop_frames = 0;
    }
    ClearAttackVolumes(scene.volumes, scene.volume_count);
    scene.layout_error.clear();
    AttachControllers(scene);
}

void RemoveLastSubject(SceneState& scene) {
    if (!CanRemoveSubject(scene)) {
        return;
    }
    scene.subject_count -= 1;
    scene.subjects[scene.subject_count].remaining = 0;
    AttachControllers(scene);
}
