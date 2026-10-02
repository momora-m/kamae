#pragma once

#include <string>
#include <vector>

struct SceneState;

constexpr int kSceneNameMax = 64;

enum class ArenaLoad {
    Missing,
    Applied,
    Rejected,
};

// Names of scenes/*.txt in the working directory, sorted. A missing folder is
// an empty list. arena.txt is not listed and is not deleted.
void ListScenes(std::vector<std::string>& names);

// Writes the current floor half and used subjects to scenes/<name>.txt.
// An unusable name or a write failure sets layout_error. Success clears it.
// Remaining, cooldown, weapon form, rounds, body size, parts, pitch, roll,
// camera, and clear color are not written.
// This does not change the code's builtin layout.
bool SaveScene(SceneState& scene, const std::string& name);

// Reads one named scene. Applied replaces the floor and subjects. Rejected and
// Missing leave the current layout and set layout_error. Loaded subjects start
// at remaining 3, cooldown 0, blade form, kGunRounds, pitch 0, and roll 0.
// Opponents are the large body with full parts. A loaded Y below the large
// body's rest height is raised to that height.
ArenaLoad LoadScene(SceneState& scene, const std::string& name);
