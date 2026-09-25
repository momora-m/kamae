#pragma once

struct SceneState;

// Edits each subject's position. The first subject keeps pitch, yaw, and roll.
// Later subjects edit yaw only. yaw_held[index] is true when that yaw slider is
// being dragged. yaw_held may be null. The values are the live fields, not a copy.
void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count);
