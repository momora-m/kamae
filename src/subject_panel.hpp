#pragma once

struct SceneState;

// During editing, edits each subject's position. The first subject keeps pitch,
// yaw, and roll. Later subjects edit yaw only. A named scene can be saved and
// chosen from the list; that load replaces the layout for the next trial.
// Start leaves editing and begins a trial. During a trial the layout controls
// and the scene list stay hidden. yaw_held[index] is true when that yaw slider
// is being dragged. yaw_held may be null. The values are the live fields, not a copy.
void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count);
