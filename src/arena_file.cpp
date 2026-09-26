#include "arena_file.hpp"

#include "attack.hpp"
#include "attack_mark.hpp"
#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kSceneDirectory = "scenes";

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
    error = "Could not read the scene. The current layout was kept.";
}

std::filesystem::path PathFromUtf8(const std::string& utf8) {
    const std::u8string bytes(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size());
    return std::filesystem::path(bytes);
}

std::string Utf8FromPath(const std::filesystem::path& path) {
    const std::u8string bytes = path.u8string();
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

bool ReservedDeviceName(const std::string& name) {
    std::string base = name;
    const std::string::size_type dot = base.find('.');
    if (dot != std::string::npos) {
        base.resize(dot);
    }
    for (char& ch : base) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    static constexpr const char* kReserved[] = {
        "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
        "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    };
    for (const char* reserved : kReserved) {
        if (base == reserved) {
            return true;
        }
    }
    return false;
}

bool NameUsable(const std::string& name) {
    if (name.empty() || name.size() > kSceneNameMax || name == "." || name == "..") {
        return false;
    }
    if (name.front() == ' ' || name.back() == ' ' || name.back() == '.') {
        return false;
    }
    for (const unsigned char ch : name) {
        if (ch < 0x20) {
            return false;
        }
        switch (ch) {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            return false;
        default:
            break;
        }
    }
    return !ReservedDeviceName(name);
}

std::filesystem::path SceneFilePath(const std::string& name) {
    return std::filesystem::path(kSceneDirectory) / PathFromUtf8(name + ".txt");
}

ArenaLoad ReadArena(const std::filesystem::path& path, ParsedArena& parsed, std::string& error) {
    std::ifstream input(path);
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
        subject.attack_buffered = false;
        subject.attack_reaction = kAttackReactionIdle;
        ClearAttackMark(scene.attack_marks[index]);
    }
    for (int index = parsed.subject_count; index < kSubjectCapacity; ++index) {
        scene.subjects[index].remaining = 0;
        scene.subjects[index].attack_cooldown = 0.0f;
        scene.subjects[index].attack_buffered = false;
        scene.subjects[index].attack_reaction = kAttackReactionIdle;
        ClearAttackMark(scene.attack_marks[index]);
    }
    scene.layout_error.clear();
}

bool WriteArena(const std::filesystem::path& path, const SceneState& scene) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
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
    return static_cast<bool>(output);
}

}  // namespace

void ListScenes(std::vector<std::string>& names) {
    names.clear();
    std::error_code error;
    const std::filesystem::path directory(kSceneDirectory);
    if (!std::filesystem::is_directory(directory, error) || error) {
        return;
    }
    std::filesystem::directory_iterator cursor(directory, error);
    if (error) {
        return;
    }
    const std::filesystem::directory_iterator end;
    for (; cursor != end; cursor.increment(error)) {
        if (error) {
            names.clear();
            return;
        }
        std::error_code file_error;
        if (!cursor->is_regular_file(file_error) || file_error) {
            continue;
        }
        const std::filesystem::path& path = cursor->path();
        std::string extension = Utf8FromPath(path.extension());
        for (char& ch : extension) {
            if (ch >= 'A' && ch <= 'Z') {
                ch = static_cast<char>(ch - 'A' + 'a');
            }
        }
        if (extension != ".txt") {
            continue;
        }
        const std::string name = Utf8FromPath(path.stem());
        if (!NameUsable(name)) {
            continue;
        }
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
}

bool SaveScene(SceneState& scene, const std::string& name) {
    if (!NameUsable(name)) {
        scene.layout_error = "The scene name is not usable.";
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(kSceneDirectory, error);
    if (error) {
        scene.layout_error = "Could not write the scene.";
        return false;
    }
    if (!WriteArena(SceneFilePath(name), scene)) {
        scene.layout_error = "Could not write the scene.";
        return false;
    }
    scene.layout_error.clear();
    return true;
}

ArenaLoad LoadScene(SceneState& scene, const std::string& name) {
    if (!NameUsable(name)) {
        scene.layout_error = "The scene name is not usable.";
        return ArenaLoad::Rejected;
    }
    ParsedArena parsed;
    std::string error;
    const ArenaLoad result = ReadArena(SceneFilePath(name), parsed, error);
    if (result == ArenaLoad::Applied) {
        ApplyArena(scene, parsed);
        return ArenaLoad::Applied;
    }
    if (result == ArenaLoad::Missing) {
        scene.layout_error = "The scene was not found.";
        return ArenaLoad::Missing;
    }
    scene.layout_error = error;
    return ArenaLoad::Rejected;
}
