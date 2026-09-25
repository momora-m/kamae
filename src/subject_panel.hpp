#pragma once

struct SceneState;

// During editing, edits each subject's position. The first subject keeps pitch,
// yaw, and roll. Later subjects edit yaw only. A named scene can be saved and
// chosen from the list; that load replaces the layout for the next trial.
// Reset to initial replaces the layout with the builtin 1v1 and leaves saved
// scenes alone. Start remembers that layout and begins a trial. A wipe stops
// the trial without restoring it. Return to start puts the remembered layout
// back and returns to editing. During a trial or after a stop, the layout
// controls, the reset, and the scene list stay hidden. yaw_held[index] is true
// when that yaw slider is being dragged. yaw_held may be null. The values are
// the live fields, not a copy.
void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count);
