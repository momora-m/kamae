#pragma once

#include "overlap.hpp"

struct Subject;

// Axis-aligned part on an opponent. Yaw is ignored. An unknown index is empty.
AxisBox PartBox(const Subject& subject, int part_index);

int IntactPartCount(const Subject& subject);
bool AnyPartIntact(const Subject& subject);

// Player: half-extent kCubeHalfExtent and no parts. Opponents: the large body
// and full part durability. Does not move the subject.
void AssignRoleBody(Subject& subject, int index);

// Unused slots. Not drawn, not an obstacle, not a part target.
void ClearRoleBody(Subject& subject);

// Raises the center when it sits below the height that puts this body's
// bottom on the player's floor. A higher center stays where it is.
void LiftBodyToFloor(Subject& subject);
