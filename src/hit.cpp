#include "hit.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "overlap.hpp"
#include "renderer.hpp"

int CollectVolumeHits(
    AttackMark& mark,
    const Subject* subjects,
    int subject_count,
    Hit* hits,
    int hit_capacity) {
    if (subjects == nullptr || hits == nullptr || subject_count <= 0 || hit_capacity <= 0) {
        return 0;
    }
    if (mark.remaining_frames <= 0 || !mark.visible) {
        return 0;
    }
    const int count = subject_count < kSubjectCapacity ? subject_count : kSubjectCapacity;
    int hit_count = 0;
    for (int index = 0; index < count; ++index) {
        if (index == mark.attacker_index) {
            continue;
        }
        const Subject& other = subjects[index];
        if (other.remaining <= 0) {
            continue;
        }
        const unsigned bit = 1u << static_cast<unsigned>(index);
        if ((mark.hit_mask & bit) != 0u) {
            continue;
        }
        if (!AxisBoxesOverlap(mark.box, SubjectBox(other))) {
            continue;
        }
        mark.hit_mask |= bit;
        if (hit_count < hit_capacity) {
            hits[hit_count].target_index = index;
            hit_count += 1;
        }
    }
    return hit_count;
}

void ApplyHit(Subject& subject) {
    if (subject.remaining <= 0) {
        return;
    }
    subject.remaining -= 1;
    subject.hitstop_frames = kHitstopFrames;
}
