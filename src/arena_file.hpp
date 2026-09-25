#pragma once

struct SceneState;

enum class ArenaLoad {
    Missing,
    Applied,
    Rejected,
};

// Reads the working directory's arena.txt. Applied replaces the floor and subjects.
// Rejected leaves the scene layout alone and sets layout_error. Missing does not
// set layout_error; the caller decides whether absence is a failure.
ArenaLoad LoadArena(SceneState& scene);

// Writes the current floor half and used subjects. Failure sets layout_error.
// Success clears it. Remaining and cooldown are not written.
bool SaveArena(SceneState& scene);
