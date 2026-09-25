#pragma once

struct SceneState;

// The list is fixed length. These change how many slots are in use.
// The first subject stays. A new subject overlaps anything already at the spawn.
bool CanAddSubject(const SceneState& scene);
bool CanRemoveSubject(const SceneState& scene);
void AddSubject(SceneState& scene);
void RemoveLastSubject(SceneState& scene);

// Replaces the floor and subjects with the code's builtin 1v1. Saved scene
// files stay. Camera, clear color, and the session stay.
void ApplyBuiltinLayout(SceneState& scene);
