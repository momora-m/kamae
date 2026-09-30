#include "trial.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "controller.hpp"
#include "hit.hpp"
#include "overlap.hpp"
#include "renderer.hpp"
#include "walk.hpp"

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
    subject.buffered_move = kMoveNone;
    subject.attack_reaction = kAttackReactionIdle;
    subject.hitstop_frames = 0;
    subject.retreating = false;
    subject.retreat_move = kMoveNone;
    subject.focus_move = kMoveNone;
    subject.swing_move = kMoveNone;
    subject.swing_elapsed = 0;
    subject.guarding = false;
    subject.velocity[0] = 0.0f;
    subject.velocity[1] = 0.0f;
    subject.velocity[2] = 0.0f;
}

void ClearUnused(SceneState& scene, int count) {
    for (int index = count; index < kSubjectCapacity; ++index) {
        scene.subjects[index].remaining = 0;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].buffered_move = kMoveNone;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        scene.subjects[index].hitstop_frames = 0;
        scene.subjects[index].retreating = false;
        scene.subjects[index].retreat_move = kMoveNone;
        scene.subjects[index].focus_move = kMoveNone;
        scene.subjects[index].swing_move = kMoveNone;
        scene.subjects[index].swing_elapsed = 0;
        scene.subjects[index].guarding = false;
        scene.subjects[index].velocity[0] = 0.0f;
        scene.subjects[index].velocity[1] = 0.0f;
        scene.subjects[index].velocity[2] = 0.0f;
    }
    ClearAttackVolumes(scene.volumes, scene.volume_count);
}

}  // namespace

float TakeSubjectFrameSeconds(Subject& subject, float frame_seconds) {
    if (subject.hitstop_frames > 0) {
        subject.hitstop_frames -= 1;
        return 0.0f;
    }
    return frame_seconds;
}

void BeginTrial(SceneState& scene) {
    const int count = UsedCount(scene.subject_count);
    scene.subject_count = count;
    scene.start_layout.floor_half = scene.floor_half;
    scene.start_layout.subject_count = count;
    for (int index = 0; index < count; ++index) {
        RememberPose(scene.start_layout.subjects[index], scene.subjects[index]);
        scene.subjects[index].remaining = 3;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].buffered_move = kMoveNone;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        scene.subjects[index].hitstop_frames = 0;
        scene.subjects[index].retreating = false;
        scene.subjects[index].retreat_move = kMoveNone;
        scene.subjects[index].focus_move = kMoveNone;
        scene.subjects[index].swing_move = kMoveNone;
        scene.subjects[index].swing_elapsed = 0;
        scene.subjects[index].guarding = false;
        scene.subjects[index].velocity[0] = 0.0f;
        scene.subjects[index].velocity[1] = 0.0f;
        scene.subjects[index].velocity[2] = 0.0f;
    }
    ClearUnused(scene, count);
    AttachControllers(scene);
    scene.session = SessionMode::Trial;
}

void RestoreStartLayout(SceneState& scene) {
    const int count = UsedCount(scene.start_layout.subject_count);
    scene.floor_half = scene.start_layout.floor_half;
    scene.subject_count = count;
    for (int index = 0; index < count; ++index) {
        ApplyRememberedPose(scene.subjects[index], scene.start_layout.subjects[index]);
    }
    ClearUnused(scene, count);
    scene.layout_error.clear();
    AttachControllers(scene);
    scene.session = SessionMode::Editing;
}

void RestartTrial(SceneState& scene) {
    const int count = UsedCount(scene.start_layout.subject_count);
    scene.floor_half = scene.start_layout.floor_half;
    scene.subject_count = count;
    for (int index = 0; index < count; ++index) {
        ApplyRememberedPose(scene.subjects[index], scene.start_layout.subjects[index]);
    }
    ClearUnused(scene, count);
    scene.layout_error.clear();
    AttachControllers(scene);
    scene.session = SessionMode::Trial;
}

TrialStop TrialStopReason(const SceneState& scene) {
    const int count = UsedCount(scene.subject_count);
    if (count <= 0) {
        return TrialStop::Continue;
    }
    if (scene.subjects[kPlayer].remaining <= 0) {
        return TrialStop::PlayerDepleted;
    }
    if (count <= kPlayer + 1) {
        return TrialStop::Continue;
    }
    for (int index = kPlayer + 1; index < count; ++index) {
        if (scene.subjects[index].remaining > 0) {
            return TrialStop::Continue;
        }
    }
    return TrialStop::OpponentsDepleted;
}

void ApplyVolumeHits(SceneState& scene) {
    Hit hits[kSubjectCapacity]{};
    const int volume_count = scene.volume_count < kVolumeCapacity ? scene.volume_count : kVolumeCapacity;
    for (int volume_index = 0; volume_index < volume_count; ++volume_index) {
        const int hit_count = CollectVolumeHits(
            scene.volumes[volume_index],
            scene.subjects,
            scene.subject_count,
            hits,
            kSubjectCapacity);
        for (int hit_index = 0; hit_index < hit_count; ++hit_index) {
            const int target_index = hits[hit_index].target_index;
            if (target_index < 0 || target_index >= scene.subject_count) {
                continue;
            }
            if (!GuardBlocksHit(scene.subjects[target_index], scene.volumes[volume_index])) {
                ApplyHit(scene.subjects[target_index]);
                AddHitVelocity(scene.subjects[target_index], scene.volumes[volume_index]);
            }
        }
    }
}

void IntegrateAndResolve(SceneState& scene, int subject_index, float frame_seconds) {
    if (subject_index < 0 || subject_index >= scene.subject_count) {
        return;
    }
    Subject& subject = scene.subjects[subject_index];
    if (subject.remaining <= 0) {
        return;
    }
    const float previous_x = subject.position[0];
    const float previous_z = subject.position[2];
    IntegrateSubject(subject, frame_seconds);
    ResolveHorizontalOverlap(
        scene.subjects, scene.subject_count, subject_index, previous_x, previous_z, scene.floor_half);
}

void ApplySubjectActions(
    SceneState& scene,
    int subject_index,
    const SubjectActions& actions,
    const HorizontalBasis& basis,
    bool face_move,
    float walk_seconds,
    float frame_seconds,
    bool buffer_early_press) {
    if (subject_index < 0 || subject_index >= scene.subject_count) {
        return;
    }
    if (scene.subjects[subject_index].remaining <= 0) {
        return;
    }

    Subject& subject = scene.subjects[subject_index];
    subject.guarding = actions.guard && subject.swing_move == kMoveNone;
    AdvanceAttack(
        scene.subjects,
        scene.subject_count,
        subject_index,
        frame_seconds,
        actions.move,
        buffer_early_press);
    if (SubjectOnFloor(subject) && !MoveLocksWalk(subject) && !subject.guarding) {
        WalkCube(subject, basis, actions.walk, walk_seconds, frame_seconds, face_move);
    }
    SpawnAttackVolume(
        scene.subjects,
        scene.subject_count,
        subject_index,
        scene.volumes,
        scene.volume_count,
        kVolumeCapacity);
}
