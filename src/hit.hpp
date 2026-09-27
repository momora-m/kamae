#pragma once

struct AttackMark;
struct Subject;

// One overlap the session may apply. The volume does not change remaining.
struct Hit {
    int target_index = -1;
};

// Reports subjects overlapping the volume. Sets hit_mask so one volume
// reports a subject once. Does not change remaining or hitstop_frames.
int CollectVolumeHits(
    AttackMark& mark,
    const Subject* subjects,
    int subject_count,
    Hit* hits,
    int hit_capacity);

// True when the defender is guarding and the volume's center sits in front
// of the defender's yaw. Side and back do not block. Does not change remaining.
bool GuardBlocksHit(const Subject& defender, const AttackMark& mark);

// Remaining loses 1. The next kHitstopFrames pass 0 seconds to walk and attack.
void ApplyHit(Subject& subject);
