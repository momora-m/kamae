#include "arena_file.hpp"

#include "attack_mark.hpp"
#include "renderer.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kArenaFile = "arena.txt";

struct ParsedSubject {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float color[3] = {0.0f, 0.0f, 0.0f};
};

struct ParsedArena {
    float floor_half = kFloorHalfExtent;
    int subject_count = 0;
    ParsedSubject subjects[kSubjectCapacity]{};
};

bool HasExtraToken(std::istringstream& row) {
    std::string extra;
    return static_cast<bool>(row >> extra);
}

bool ColorInRange(float channel) {
    return std::isfinite(channel) && channel >= 0.0f && channel <= 1.0f;
}

void Reject(std::string& error) {
    error = "Could not read arena.txt. The current layout was kept.";
}

ArenaLoad ReadArena(ParsedArena& parsed, std::string& error) {
    std::ifstream input(kArenaFile);
    if (!input) {
        return ArenaLoad::Missing;
    }

    std::string line;
    bool saw_floor = false;
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        std::istringstream row(line);
        if (!saw_floor) {
            std::string tag;
            float half = 0.0f;
            row >> tag >> half;
            if (tag != "floor" || row.fail() || HasExtraToken(row) || !std::isfinite(half) || half < kFloorHalfMin ||
                half > kFloorHalfMax) {
                Reject(error);
                return ArenaLoad::Rejected;
            }
            parsed.floor_half = half;
            saw_floor = true;
            continue;
        }

        if (parsed.subject_count >= kSubjectCapacity) {
            Reject(error);
            return ArenaLoad::Rejected;
        }

        ParsedSubject subject;
        row >> subject.position[0] >> subject.position[1] >> subject.position[2] >> subject.yaw >> subject.color[0] >>
            subject.color[1] >> subject.color[2];
        if (row.fail() || HasExtraToken(row) || !std::isfinite(subject.position[0]) ||
            !std::isfinite(subject.position[1]) || !std::isfinite(subject.position[2]) || !std::isfinite(subject.yaw) ||
            !ColorInRange(subject.color[0]) || !ColorInRange(subject.color[1]) || !ColorInRange(subject.color[2])) {
            Reject(error);
            return ArenaLoad::Rejected;
        }
        parsed.subjects[parsed.subject_count] = subject;
        parsed.subject_count += 1;
    }

    if (input.bad() || !saw_floor || parsed.subject_count < 1) {
        Reject(error);
        return ArenaLoad::Rejected;
    }
    return ArenaLoad::Applied;
}

void ApplyArena(SceneState& scene, const ParsedArena& parsed) {
    scene.floor_half = parsed.floor_half;
    scene.subject_count = parsed.subject_count;
    for (int index = 0; index < parsed.subject_count; ++index) {
        Subject& subject = scene.subjects[index];
        const ParsedSubject& source = parsed.subjects[index];
        subject.position[0] = source.position[0];
        subject.position[1] = source.position[1];
        subject.position[2] = source.position[2];
        subject.rotation_degrees[0] = 0.0f;
        subject.rotation_degrees[1] = source.yaw;
        subject.rotation_degrees[2] = 0.0f;
        subject.color[0] = source.color[0];
        subject.color[1] = source.color[1];
        subject.color[2] = source.color[2];
        subject.remaining = 3;
        subject.attack_cooldown = 0.0f;
        ClearAttackMark(scene.attack_marks[index]);
    }
    for (int index = parsed.subject_count; index < kSubjectCapacity; ++index) {
        scene.subjects[index].remaining = 0;
        scene.subjects[index].attack_cooldown = 0.0f;
        ClearAttackMark(scene.attack_marks[index]);
    }
    scene.layout_error.clear();
}

}  // namespace

ArenaLoad LoadArena(SceneState& scene) {
    ParsedArena parsed;
    std::string error;
    const ArenaLoad result = ReadArena(parsed, error);
    if (result == ArenaLoad::Applied) {
        ApplyArena(scene, parsed);
        return ArenaLoad::Applied;
    }
    if (result == ArenaLoad::Rejected) {
        scene.layout_error = error;
    }
    return result;
}

bool SaveArena(SceneState& scene) {
    std::ofstream output(kArenaFile, std::ios::trunc);
    if (!output) {
        scene.layout_error = "Could not write arena.txt.";
        return false;
    }
    output.precision(9);
    output << "floor " << scene.floor_half << '\n';
    int count = scene.subject_count;
    if (count < 0) {
        count = 0;
    }
    if (count > kSubjectCapacity) {
        count = kSubjectCapacity;
    }
    for (int index = 0; index < count; ++index) {
        const Subject& subject = scene.subjects[index];
        output << subject.position[0] << ' ' << subject.position[1] << ' ' << subject.position[2] << ' '
               << subject.rotation_degrees[1] << ' ' << subject.color[0] << ' ' << subject.color[1] << ' '
               << subject.color[2] << '\n';
    }
    if (!output) {
        scene.layout_error = "Could not write arena.txt.";
        return false;
    }
    scene.layout_error.clear();
    return true;
}
