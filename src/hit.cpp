#include "hit.hpp"

#include "actions.hpp"
#include "attack.hpp"
#include "attack_mark.hpp"
#include "overlap.hpp"
#include "parts.hpp"
#include "renderer.hpp"

#include <cmath>
#include <numbers>

static_assert(kHitForce == 1.0f, "hit force is the smallest speed that opens 0.4 in 0.4 seconds");

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

bool GuardBlocksHit(const Subject& defender, const AttackMark& mark) {
    if (!defender.guarding) {
        return false;
    }
    const float to_x = (mark.box.min_x + mark.box.max_x) * 0.5f - defender.position[0];
    const float to_z = (mark.box.min_z + mark.box.max_z) * 0.5f - defender.position[2];
    const float yaw = defender.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    const float forward_x = std::sin(yaw);
    const float forward_z = std::cos(yaw);
    return to_x * forward_x + to_z * forward_z > 0.0f;
}

namespace {

unsigned PartBit(int subject_index, int part_index) {
    return 1u << static_cast<unsigned>(subject_index * kPartCount + part_index);
}

void RestoreOneRound(Subject& subject) {
    if (subject.rounds < kGunRounds) {
        subject.rounds += 1;
    }
}

}  // namespace

void ApplyOneVolume(AttackMark& mark, Subject* subjects, int subject_count) {
    if (subjects == nullptr || subject_count <= 0) {
        return;
    }
    if (mark.remaining_frames <= 0 || !mark.visible) {
        return;
    }
    const int count = subject_count < kSubjectCapacity ? subject_count : kSubjectCapacity;
    for (int index = 0; index < count; ++index) {
        if (index == mark.attacker_index) {
            continue;
        }
        Subject& subject = subjects[index];
        if (subject.remaining <= 0) {
            continue;
        }
        const bool blocked = subject.dodge_frames > 0 || GuardBlocksHit(subject, mark);
        const bool body_was_open = !AnyPartIntact(subject);
        bool broke_part = false;
        for (int part = 0; part < kPartCount; ++part) {
            if (subject.part_remaining[part] <= 0) {
                continue;
            }
            const unsigned bit = PartBit(index, part);
            if ((mark.part_mask & bit) != 0u) {
                continue;
            }
            if (!AxisBoxesOverlap(mark.box, PartBox(subject, part))) {
                continue;
            }
            mark.part_mask |= bit;
            if (blocked) {
                continue;
            }
            subject.part_remaining[part] -= 1;
            broke_part = true;
        }

        const unsigned body_bit = 1u << static_cast<unsigned>(index);
        bool new_body = false;
        if ((mark.hit_mask & body_bit) == 0u && AxisBoxesOverlap(mark.box, SubjectBox(subject))) {
            mark.hit_mask |= body_bit;
            new_body = true;
        }
        if (blocked) {
            continue;
        }
        if (mark.restores_round) {
            if (broke_part || (new_body && body_was_open)) {
                if (mark.attacker_index == kPlayer && mark.attacker_index >= 0 &&
                    mark.attacker_index < count) {
                    RestoreOneRound(subjects[mark.attacker_index]);
                }
            }
            continue;
        }
        if (new_body && body_was_open) {
            ApplyHit(subject);
            AddHitVelocity(subject, mark);
        }
    }
}

void ApplyHit(Subject& subject) {
    if (subject.remaining <= 0) {
        return;
    }
    subject.remaining -= 1;
    subject.hitstop_frames = kHitstopFrames;
}

void AddHitVelocity(Subject& subject, const AttackMark& mark) {
    subject.velocity[0] += mark.forward_x * kHitForce;
    subject.velocity[2] += mark.forward_z * kHitForce;
}
