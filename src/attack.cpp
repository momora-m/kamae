#include "attack.hpp"

#include "actions.hpp"
#include "attack_mark.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr MoveRow kPoke{
    1.0f,
    0.5f,
    kPlayerAttackInterval,
    kMoveStartupFrames,
    kMoveActiveFrames,
    kMoveRecoveryFrames};
constexpr MoveRow kLong{
    kLongMoveForward,
    0.5f,
    kLongMoveInterval,
    kMoveStartupFrames,
    kMoveActiveFrames,
    kMoveRecoveryFrames};
constexpr MoveRow kGun{
    kGunForward,
    0.5f,
    kGunInterval,
    kMoveStartupFrames,
    kMoveActiveFrames,
    kMoveRecoveryFrames};

static_assert(kSubjectCapacity <= 32, "the volume hit mask has one bit per subject");
static_assert(kPoke.interval == 0.4f, "the poke waits 0.4 seconds");
static_assert(kLong.forward > kPoke.forward, "the long move reaches farther");
static_assert(kLong.interval > kPoke.interval, "the long move waits longer");
static_assert(kPoke.startup_frames == 1, "startup is the smallest positive lock");
static_assert(kPoke.active_frames == 3, "active stays three frames");
static_assert(kPoke.recovery_frames == 1, "recovery is the smallest positive lock");
static_assert(kLong.startup_frames == kPoke.startup_frames, "both blade rows share startup");
static_assert(kLong.active_frames == kPoke.active_frames, "both blade rows share active");
static_assert(kLong.recovery_frames == kPoke.recovery_frames, "both blade rows share recovery");
static_assert(kGun.forward > kLong.forward, "the gun reaches farther than the long move");
static_assert(kGun.lateral == kPoke.lateral, "only the gun's forward reach changes");
static_assert(kGun.interval > kPoke.interval && kGun.interval < kLong.interval, "gun cadence sits between the blades");
static_assert(kGun.interval > kAttackBufferWindow, "the buffer window fits in the gun interval");
static_assert(kGun.startup_frames == kPoke.startup_frames, "the gun shares startup");
static_assert(kGun.active_frames == kPoke.active_frames, "the gun shares active");
static_assert(kGun.recovery_frames == kPoke.recovery_frames, "the gun shares recovery");
// Builtin opponent sits at z = 4. Shared faces do not overlap, so the gun's
// far face must pass the opponent's near face. The long move still falls short.
static_assert(kCubeHalfExtent + kGunForward > 4.0f - kCubeHalfExtent, "the gun overlaps the builtin spacing of 4");
static_assert(
    kCubeHalfExtent + kLongMoveForward <= 4.0f - kCubeHalfExtent,
    "the long move still misses the builtin spacing of 4");
static_assert(kGunRounds == 3, "three rounds show a spent shot before empty");
static_assert(
    kAttackVolumeActiveFrames == kMoveActiveFrames, "the volume lasts the move's active window");

void ClearSwing(Subject& subject) {
    subject.swing_move = kMoveNone;
    subject.swing_elapsed = 0;
}

int SwingLength(const MoveRow& row) {
    return row.startup_frames + row.active_frames + row.recovery_frames;
}

bool InStartup(int elapsed, const MoveRow& row) {
    return elapsed < row.startup_frames;
}

bool InRecovery(int elapsed, const MoveRow& row) {
    const int active_end = row.startup_frames + row.active_frames;
    return elapsed >= active_end && elapsed < SwingLength(row);
}

// Blade rows only on the blade. The gun only in gun form, and only with a round left.
bool AcceptsMove(const Subject& attacker, int move_id) {
    if (move_id == kMoveGun) {
        return attacker.weapon_form == WeaponForm::Gun && attacker.rounds > 0;
    }
    if (move_id == kMovePoke || move_id == kMoveLong) {
        return attacker.weapon_form == WeaponForm::Blade;
    }
    return false;
}

}  // namespace

static_assert(kAttackReactionIdle < 0.0f, "an idle reaction is not a finished wait");
static_assert(
    Subject{}.attack_reaction == kAttackReactionIdle, "a new subject has not started the in-range wait");
static_assert(Subject{}.swing_move == kMoveNone, "a new subject is not in a swing");
static_assert(Subject{}.swing_elapsed == 0, "a new subject has no swing frames");
static_assert(!Subject{}.guarding, "a new subject is not guarding");
static_assert(Subject{}.velocity[0] == 0.0f && Subject{}.velocity[1] == 0.0f && Subject{}.velocity[2] == 0.0f,
    "a new subject has no leftover velocity");
static_assert(Subject{}.weapon_form == WeaponForm::Blade, "a new subject starts on the blade");
static_assert(Subject{}.rounds == kGunRounds, "a new subject starts with a full gun");

bool TryMove(int move_id, MoveRow& row) {
    if (move_id == kMovePoke) {
        row = kPoke;
        return true;
    }
    if (move_id == kMoveLong) {
        row = kLong;
        return true;
    }
    if (move_id == kMoveGun) {
        row = kGun;
        return true;
    }
    return false;
}

bool MoveLocksWalk(const Subject& subject) {
    MoveRow row{};
    if (!TryMove(subject.swing_move, row)) {
        return false;
    }
    return InStartup(subject.swing_elapsed, row) || InRecovery(subject.swing_elapsed, row);
}

// Yaw 0 faces +Z. The volume starts at the front face and extends the move's forward
// reach, with the move's lateral half-width. Off-axis yaw uses the corners' bounds.
AxisBox AttackBox(const Subject& attacker, int move_id) {
    MoveRow row{};
    const bool known = TryMove(move_id, row);
    const float forward = known ? row.forward : 0.0f;
    const float lateral = known ? row.lateral : 0.0f;
    const float yaw = attacker.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    const float forward_x = std::sin(yaw);
    const float forward_z = std::cos(yaw);
    const float right_x = std::cos(yaw);
    const float right_z = -std::sin(yaw);
    const float forward_steps[2] = {kCubeHalfExtent, kCubeHalfExtent + forward};
    const float lateral_steps[2] = {-lateral, lateral};

    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
    for (int forward_index = 0; forward_index < 2; ++forward_index) {
        for (int lateral_index = 0; lateral_index < 2; ++lateral_index) {
            const float x = attacker.position[0] + forward_x * forward_steps[forward_index] +
                            right_x * lateral_steps[lateral_index];
            const float z = attacker.position[2] + forward_z * forward_steps[forward_index] +
                            right_z * lateral_steps[lateral_index];
            if (forward_index == 0 && lateral_index == 0) {
                min_x = max_x = x;
                min_z = max_z = z;
                continue;
            }
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
            min_z = std::min(min_z, z);
            max_z = std::max(max_z, z);
        }
    }

    return AxisBox{
        min_x,
        attacker.position[1] - kCubeHalfExtent,
        min_z,
        max_x,
        attacker.position[1] + kCubeHalfExtent,
        max_z,
    };
}

void TickAttackVolume(AttackMark& mark) {
    if (mark.remaining_frames <= 0) {
        ClearAttackMark(mark);
        return;
    }
    mark.remaining_frames -= 1;
    if (mark.remaining_frames <= 0) {
        ClearAttackMark(mark);
        return;
    }
    mark.visible = true;
}

void TickAttackVolumes(AttackMark* volumes, int& volume_count) {
    if (volumes == nullptr || volume_count <= 0) {
        volume_count = 0;
        return;
    }
    int kept = 0;
    for (int index = 0; index < volume_count; ++index) {
        TickAttackVolume(volumes[index]);
        if (volumes[index].remaining_frames <= 0) {
            continue;
        }
        if (kept != index) {
            volumes[kept] = volumes[index];
        }
        kept += 1;
    }
    for (int index = kept; index < volume_count; ++index) {
        ClearAttackMark(volumes[index]);
    }
    volume_count = kept;
}

void AdvanceAttack(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    float frame_seconds,
    int move_id,
    bool buffer_early_press) {
    if (subjects == nullptr || attacker_index < 0 || attacker_index >= subject_count) {
        return;
    }

    Subject& attacker = subjects[attacker_index];
    attacker.attack_cooldown = AdvanceAttackTimer(attacker.attack_cooldown, frame_seconds);
    if (attacker.remaining <= 0) {
        attacker.buffered_move = kMoveNone;
        ClearSwing(attacker);
        return;
    }

    if (attacker.swing_move != kMoveNone) {
        MoveRow swing_row{};
        if (!TryMove(attacker.swing_move, swing_row)) {
            ClearSwing(attacker);
        } else {
            attacker.swing_elapsed += 1;
            if (attacker.swing_elapsed >= SwingLength(swing_row)) {
                ClearSwing(attacker);
            }
        }
    }

    // A remembered move from the other form, or a gun with no rounds, does not fire later.
    if (attacker.buffered_move != kMoveNone && !AcceptsMove(attacker, attacker.buffered_move)) {
        attacker.buffered_move = kMoveNone;
    }
    // Judged after this frame's tick. A press earlier than the window is not stored.
    if (buffer_early_press && AcceptsMove(attacker, move_id) && attacker.attack_cooldown > 0.0f &&
        attacker.attack_cooldown <= kAttackBufferWindow) {
        attacker.buffered_move = move_id;
    }
    int fired = kMoveNone;
    if (attacker.attack_cooldown <= 0.0f && attacker.swing_move == kMoveNone && !attacker.guarding) {
        if (AcceptsMove(attacker, move_id)) {
            fired = move_id;
        } else if (buffer_early_press && AcceptsMove(attacker, attacker.buffered_move)) {
            fired = attacker.buffered_move;
        }
    }
    MoveRow row{};
    if (fired == kMoveNone || !TryMove(fired, row)) {
        return;
    }
    attacker.buffered_move = kMoveNone;
    attacker.attack_cooldown = row.interval;
    attacker.swing_move = fired;
    attacker.swing_elapsed = 0;
    if (fired == kMoveGun) {
        attacker.rounds -= 1;
    }
}

void SpawnAttackVolume(
    Subject* subjects,
    int subject_count,
    int attacker_index,
    AttackMark* volumes,
    int& volume_count,
    int volume_capacity) {
    if (subjects == nullptr || attacker_index < 0 || attacker_index >= subject_count) {
        return;
    }
    const Subject& attacker = subjects[attacker_index];
    if (attacker.remaining <= 0) {
        return;
    }
    MoveRow row{};
    if (!TryMove(attacker.swing_move, row) || row.active_frames <= 0) {
        return;
    }
    if (attacker.swing_elapsed != row.startup_frames) {
        return;
    }
    if (volumes == nullptr || volume_count < 0 || volume_count >= volume_capacity) {
        return;
    }
    AttackMark& volume = volumes[volume_count];
    ShowAttackMark(volume, AttackBox(attacker, attacker.swing_move), attacker_index);
    volume.remaining_frames = row.active_frames;
    const float yaw = attacker.rotation_degrees[1] * (std::numbers::pi_v<float> / 180.0f);
    volume.forward_x = std::sin(yaw);
    volume.forward_z = std::cos(yaw);
    volume_count += 1;
}
