#pragma once

struct SceneState;

// During editing, edits each subject's position. The first subject keeps pitch,
// yaw, and roll. Later subjects edit yaw only. Start remembers that layout and
// begins a trial. A wipe stops the trial without restoring it. Return to start
// puts the remembered layout back and returns to editing. During a trial or
// after a stop, the layout controls stay hidden. A failed scene load or save
// still draws scene.layout_error here. yaw_held[index] is true when that yaw
// slider is being dragged. yaw_held may be null. The values are the live
// fields, not a copy.
void ShowSubjectPanel(SceneState& scene, bool* yaw_held, int yaw_held_count);

// Main menu bar. Scene holds the name field, save, the saved-scene list, and
// reset to the builtin 1v1. Those call SaveScene, ListScenes, LoadScene, and
// ApplyBuiltinLayout. The menu is disabled while the session is not editing,
// so a trial cannot switch scenes. Submit this before the dock host. The bar
// reserves the top of the main viewport work area the host already uses.
void ShowSceneMenu(SceneState& scene);
