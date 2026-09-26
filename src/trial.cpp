#include "trial.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "renderer.hpp"

namespace {

int UsedCount(int subject_count) {
    if (subject_count < 0) {
        return 0;
    }
    if (subject_count > kSubjectCapacity) {
        return kSubjectCapacity;
    }
    return subject_count;
}

void RememberPose(SubjectPose& pose, const Subject& subject) {
    pose.position[0] = subject.position[0];
    pose.position[1] = subject.position[1];
    pose.position[2] = subject.position[2];
    pose.rotation_degrees[0] = subject.rotation_degrees[0];
    pose.rotation_degrees[1] = subject.rotation_degrees[1];
    pose.rotation_degrees[2] = subject.rotation_degrees[2];
    pose.color[0] = subject.color[0];
    pose.color[1] = subject.color[1];
    pose.color[2] = subject.color[2];
}

void ApplyRememberedPose(Subject& subject, const SubjectPose& pose) {
    subject.position[0] = pose.position[0];
    subject.position[1] = pose.position[1];
    subject.position[2] = pose.position[2];
    subject.rotation_degrees[0] = pose.rotation_degrees[0];
    subject.rotation_degrees[1] = pose.rotation_degrees[1];
    subject.rotation_degrees[2] = pose.rotation_degrees[2];
    subject.color[0] = pose.color[0];
    subject.color[1] = pose.color[1];
    subject.color[2] = pose.color[2];
    subject.remaining = 3;
    subject.attack_cooldown = 0.0f;
    subject.attack_buffered = false;
    subject.attack_reaction = kAttackReactionIdle;
}

void ClearUnused(SceneState& scene, int count) {
    for (int index = count; index < kSubjectCapacity; ++index) {
        scene.subjects[index].remaining = 0;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].attack_buffered = false;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        ClearAttackMark(scene.attack_marks[index]);
    }
}

}  // namespace

void BeginTrial(SceneState& scene) {
    const int count = UsedCount(scene.subject_count);
    scene.subject_count = count;
    scene.start_layout.floor_half = scene.floor_half;
    scene.start_layout.subject_count = count;
    for (int index = 0; index < count; ++index) {
        RememberPose(scene.start_layout.subjects[index], scene.subjects[index]);
        scene.subjects[index].remaining = 3;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].attack_buffered = false;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        ClearAttackMark(scene.attack_marks[index]);
    }
    ClearUnused(scene, count);
    scene.session = SessionMode::Trial;
}

void RestoreStartLayout(SceneState& scene) {
    const int count = UsedCount(scene.start_layout.subject_count);
    scene.floor_half = scene.start_layout.floor_half;
    scene.subject_count = count;
    for (int index = 0; index < count; ++index) {
        ApplyRememberedPose(scene.subjects[index], scene.start_layout.subjects[index]);
        ClearAttackMark(scene.attack_marks[index]);
    }
    ClearUnused(scene, count);
    scene.layout_error.clear();
    scene.session = SessionMode::Editing;
}

bool TrialShouldStop(const SceneState& scene) {
    const int count = UsedCount(scene.subject_count);
    if (count <= 0) {
        return false;
    }
    if (scene.subjects[kPlayer].remaining <= 0) {
        return true;
    }
    if (count <= kPlayer + 1) {
        return false;
    }
    for (int index = kPlayer + 1; index < count; ++index) {
        if (scene.subjects[index].remaining > 0) {
            return false;
        }
    }
    return true;
}
