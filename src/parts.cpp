#include "parts.hpp"

#include "renderer.hpp"

namespace {

AxisBox BoxFromCenter(float x, float y, float z, float half) {
    return AxisBox{x - half, y - half, z - half, x + half, y + half, z + half};
}

}  // namespace

static_assert(kPartCount == 2, "a front part and a side part");
static_assert(kSubjectCapacity * kPartCount <= 32, "the volume part mask has one bit per part");

AxisBox PartBox(const Subject& subject, int part_index) {
    float offset_x = 0.0f;
    float offset_z = 0.0f;
    if (part_index == 0) {
        offset_z = kFrontPartOffsetZ;
    } else if (part_index == 1) {
        offset_x = kSidePartOffsetX;
    } else {
        return BoxFromCenter(subject.position[0], subject.position[1], subject.position[2], 0.0f);
    }
    return BoxFromCenter(
        subject.position[0] + offset_x,
        subject.position[1] + kPartOffsetY,
        subject.position[2] + offset_z,
        kPartHalf);
}

int IntactPartCount(const Subject& subject) {
    int count = 0;
    for (int part = 0; part < kPartCount; ++part) {
        if (subject.part_remaining[part] > 0) {
            count += 1;
        }
    }
    return count;
}

bool AnyPartIntact(const Subject& subject) {
    return IntactPartCount(subject) > 0;
}

void AssignRoleBody(Subject& subject, int index) {
    if (index == kPlayer) {
        subject.body_half = kCubeHalfExtent;
        for (int part = 0; part < kPartCount; ++part) {
            subject.part_remaining[part] = 0;
        }
        return;
    }
    subject.body_half = kOpponentBodyHalf;
    for (int part = 0; part < kPartCount; ++part) {
        subject.part_remaining[part] = kPartDurability;
    }
}

void ClearRoleBody(Subject& subject) {
    subject.body_half = kCubeHalfExtent;
    for (int part = 0; part < kPartCount; ++part) {
        subject.part_remaining[part] = 0;
    }
}

void LiftBodyToFloor(Subject& subject) {
    const float rest = BodyRestY(subject.body_half);
    if (subject.position[1] < rest) {
        subject.position[1] = rest;
    }
}
