#include "mesh_file.hpp"

#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t kMaxJsonBytes = 1u * 1024u * 1024u;
constexpr std::size_t kMaxBufferBytes = 4u * 1024u * 1024u;
constexpr int kMaxJsonDepth = 32;
constexpr float kBoundSlack = 0.0001f;

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
};

struct Json {
    JsonType type = JsonType::Null;
    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::vector<Json> array;
    std::vector<std::pair<std::string, Json>> object;

    const Json* Find(const std::string& key) const {
        if (type != JsonType::Object) {
            return nullptr;
        }
        for (const auto& entry : object) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }
};

struct Parser {
    const std::string& text;
    std::size_t index = 0;
    int depth = 0;
    bool failed = false;

    explicit Parser(const std::string& source) : text(source) {}

    void Fail() { failed = true; }

    void Skip() {
        while (index < text.size()) {
            const char ch = text[index];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
                break;
            }
            ++index;
        }
    }

    bool Consume(char expected) {
        Skip();
        if (index >= text.size() || text[index] != expected) {
            Fail();
            return false;
        }
        ++index;
        return true;
    }

    Json ParseValue() {
        Skip();
        if (failed || index >= text.size()) {
            Fail();
            return {};
        }
        const char ch = text[index];
        if (ch == '{') {
            return ParseObject();
        }
        if (ch == '[') {
            return ParseArray();
        }
        if (ch == '"') {
            return ParseString();
        }
        if (ch == 't') {
            return ParseLiteral("true", JsonType::Bool, true);
        }
        if (ch == 'f') {
            return ParseLiteral("false", JsonType::Bool, false);
        }
        if (ch == 'n') {
            return ParseLiteral("null", JsonType::Null, false);
        }
        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            return ParseNumber();
        }
        Fail();
        return {};
    }

    Json ParseLiteral(const char* literal, JsonType type, bool boolean) {
        for (const char* cursor = literal; *cursor != '\0'; ++cursor) {
            if (index >= text.size() || text[index] != *cursor) {
                Fail();
                return {};
            }
            ++index;
        }
        Json value;
        value.type = type;
        value.boolean = boolean;
        return value;
    }

    Json ParseNumber() {
        const std::size_t begin = index;
        if (text[index] == '-') {
            ++index;
        }
        if (index >= text.size() || text[index] < '0' || text[index] > '9') {
            Fail();
            return {};
        }
        if (text[index] == '0') {
            ++index;
        } else {
            while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
                ++index;
            }
        }
        if (index < text.size() && text[index] == '.') {
            ++index;
            if (index >= text.size() || text[index] < '0' || text[index] > '9') {
                Fail();
                return {};
            }
            while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
                ++index;
            }
        }
        if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
            ++index;
            if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
                ++index;
            }
            if (index >= text.size() || text[index] < '0' || text[index] > '9') {
                Fail();
                return {};
            }
            while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
                ++index;
            }
        }
        double number = 0.0;
        const auto result = std::from_chars(text.data() + begin, text.data() + index, number);
        if (result.ec != std::errc() || result.ptr != text.data() + index || !std::isfinite(number)) {
            Fail();
            return {};
        }
        Json value;
        value.type = JsonType::Number;
        value.number = number;
        return value;
    }

    void AppendUtf8(std::string& out, unsigned code) {
        if (code <= 0x7F) {
            out.push_back(static_cast<char>(code));
            return;
        }
        if (code <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            return;
        }
        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }

    Json ParseString() {
        if (!Consume('"')) {
            return {};
        }
        Json value;
        value.type = JsonType::String;
        while (index < text.size()) {
            const unsigned char ch = static_cast<unsigned char>(text[index]);
            if (ch == '"') {
                ++index;
                return value;
            }
            if (ch < 0x20) {
                Fail();
                return {};
            }
            if (ch != '\\') {
                value.text.push_back(static_cast<char>(ch));
                ++index;
                continue;
            }
            ++index;
            if (index >= text.size()) {
                Fail();
                return {};
            }
            const char escaped = text[index++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                value.text.push_back(escaped);
                break;
            case 'b':
                value.text.push_back('\b');
                break;
            case 'f':
                value.text.push_back('\f');
                break;
            case 'n':
                value.text.push_back('\n');
                break;
            case 'r':
                value.text.push_back('\r');
                break;
            case 't':
                value.text.push_back('\t');
                break;
            case 'u': {
                unsigned code = 0;
                for (int nibble = 0; nibble < 4; ++nibble) {
                    if (index >= text.size()) {
                        Fail();
                        return {};
                    }
                    const char hex = text[index++];
                    code <<= 4;
                    if (hex >= '0' && hex <= '9') {
                        code |= static_cast<unsigned>(hex - '0');
                    } else if (hex >= 'a' && hex <= 'f') {
                        code |= static_cast<unsigned>(hex - 'a' + 10);
                    } else if (hex >= 'A' && hex <= 'F') {
                        code |= static_cast<unsigned>(hex - 'A' + 10);
                    } else {
                        Fail();
                        return {};
                    }
                }
                if (code >= 0xD800 && code <= 0xDFFF) {
                    Fail();
                    return {};
                }
                AppendUtf8(value.text, code);
                break;
            }
            default:
                Fail();
                return {};
            }
        }
        Fail();
        return {};
    }

    Json ParseArray() {
        if (depth >= kMaxJsonDepth || !Consume('[')) {
            Fail();
            return {};
        }
        ++depth;
        Json value;
        value.type = JsonType::Array;
        Skip();
        if (index < text.size() && text[index] == ']') {
            ++index;
            --depth;
            return value;
        }
        while (!failed) {
            value.array.push_back(ParseValue());
            if (failed) {
                break;
            }
            Skip();
            if (index < text.size() && text[index] == ',') {
                ++index;
                continue;
            }
            if (index < text.size() && text[index] == ']') {
                ++index;
                --depth;
                return value;
            }
            Fail();
            break;
        }
        return {};
    }

    Json ParseObject() {
        if (depth >= kMaxJsonDepth || !Consume('{')) {
            Fail();
            return {};
        }
        ++depth;
        Json value;
        value.type = JsonType::Object;
        Skip();
        if (index < text.size() && text[index] == '}') {
            ++index;
            --depth;
            return value;
        }
        while (!failed) {
            Skip();
            Json key = ParseString();
            if (failed || key.type != JsonType::String || !Consume(':')) {
                Fail();
                break;
            }
            Json child = ParseValue();
            if (failed) {
                break;
            }
            value.object.emplace_back(std::move(key.text), std::move(child));
            Skip();
            if (index < text.size() && text[index] == ',') {
                ++index;
                continue;
            }
            if (index < text.size() && text[index] == '}') {
                ++index;
                --depth;
                return value;
            }
            Fail();
            break;
        }
        return {};
    }
};

bool ParseJson(const std::string& text, Json& root) {
    Parser parser(text);
    root = parser.ParseValue();
    parser.Skip();
    if (parser.failed || parser.index != text.size() || root.type != JsonType::Object) {
        return false;
    }
    return true;
}

bool AsInt(const Json* value, int& out) {
    if (value == nullptr || value->type != JsonType::Number || !std::isfinite(value->number)) {
        return false;
    }
    const double rounded = std::nearbyint(value->number);
    if (std::fabs(value->number - rounded) > 1e-6) {
        return false;
    }
    if (rounded < static_cast<double>(std::numeric_limits<int>::min()) ||
        rounded > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    out = static_cast<int>(rounded);
    return true;
}

bool AsString(const Json* value, std::string& out) {
    if (value == nullptr || value->type != JsonType::String) {
        return false;
    }
    out = value->text;
    return true;
}

void Fail(std::string& error, const char* message) {
    error = message;
}

bool ReadFileBytes(const std::filesystem::path& path, std::size_t max_bytes, std::vector<unsigned char>& bytes, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        Fail(error, "Could not open the character mesh.");
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0 || static_cast<unsigned long long>(end) > max_bytes) {
        Fail(error, "Could not read the character mesh.");
        return false;
    }
    input.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(end));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!input) {
        Fail(error, "Could not read the character mesh.");
        return false;
    }
    return true;
}

bool SafeBufferName(const std::string& name) {
    if (name.empty() || name.size() > 128 || name == "." || name == "..") {
        return false;
    }
    if (name.find("..") != std::string::npos) {
        return false;
    }
    for (const unsigned char ch : name) {
        const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '.' ||
                        ch == '_' || ch == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

int ComponentSize(int component_type) {
    switch (component_type) {
    case 5120:
    case 5121:
        return 1;
    case 5122:
    case 5123:
        return 2;
    case 5125:
    case 5126:
        return 4;
    default:
        return 0;
    }
}

int ElementCount(const std::string& type) {
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    return 0;
}

struct AccessorView {
    const std::vector<unsigned char>* bytes = nullptr;
    std::uint64_t offset = 0;
    int count = 0;
    int component_type = 0;
    int components = 0;
    int stride = 0;
};

bool ResolveAccessor(
    const Json& root,
    const std::vector<std::vector<unsigned char>>& buffers,
    int accessor_index,
    AccessorView& view,
    std::string& error) {
    const Json* accessors = root.Find("accessors");
    if (accessors == nullptr || accessors->type != JsonType::Array || accessor_index < 0 ||
        static_cast<std::size_t>(accessor_index) >= accessors->array.size()) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const Json& accessor = accessors->array[static_cast<std::size_t>(accessor_index)];
    if (accessor.type != JsonType::Object || accessor.Find("sparse") != nullptr) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    if (const Json* normalized = accessor.Find("normalized");
        normalized != nullptr && !(normalized->type == JsonType::Bool && !normalized->boolean)) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    int buffer_view_index = 0;
    int component_type = 0;
    int count = 0;
    std::string type_name;
    if (!AsInt(accessor.Find("bufferView"), buffer_view_index) || !AsInt(accessor.Find("componentType"), component_type) ||
        !AsInt(accessor.Find("count"), count) || !AsString(accessor.Find("type"), type_name) || count < 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const int component_size = ComponentSize(component_type);
    const int components = ElementCount(type_name);
    if (component_size == 0 || components == 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    int accessor_offset = 0;
    if (const Json* offset = accessor.Find("byteOffset"); offset != nullptr && !AsInt(offset, accessor_offset)) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    if (accessor_offset < 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }

    const Json* views = root.Find("bufferViews");
    if (views == nullptr || views->type != JsonType::Array ||
        static_cast<std::size_t>(buffer_view_index) >= views->array.size()) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const Json& buffer_view = views->array[static_cast<std::size_t>(buffer_view_index)];
    int buffer_index = 0;
    int view_length = 0;
    if (buffer_view.type != JsonType::Object || !AsInt(buffer_view.Find("buffer"), buffer_index) ||
        !AsInt(buffer_view.Find("byteLength"), view_length) || buffer_index < 0 || view_length < 0 ||
        static_cast<std::size_t>(buffer_index) >= buffers.size()) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    int view_offset = 0;
    if (const Json* offset = buffer_view.Find("byteOffset"); offset != nullptr && !AsInt(offset, view_offset)) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    if (view_offset < 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const int element_bytes = component_size * components;
    int stride = element_bytes;
    if (const Json* byte_stride = buffer_view.Find("byteStride"); byte_stride != nullptr) {
        if (!AsInt(byte_stride, stride) || stride < element_bytes || stride % component_size != 0) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
    }
    const std::uint64_t start = static_cast<std::uint64_t>(view_offset) + static_cast<std::uint64_t>(accessor_offset);
    if (start % static_cast<unsigned>(component_size) != 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const std::uint64_t view_end = static_cast<std::uint64_t>(view_offset) + static_cast<std::uint64_t>(view_length);
    if (count > 0) {
        const std::uint64_t last = start + static_cast<std::uint64_t>(stride) * static_cast<std::uint64_t>(count - 1) +
                                   static_cast<std::uint64_t>(element_bytes);
        if (last > buffers[static_cast<std::size_t>(buffer_index)].size() || last > view_end) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
    }
    view.bytes = &buffers[static_cast<std::size_t>(buffer_index)];
    view.offset = start;
    view.count = count;
    view.component_type = component_type;
    view.components = components;
    view.stride = stride;
    return true;
}

bool ReadFloat3(const AccessorView& view, int index, float out[3], std::string& error) {
    if (view.bytes == nullptr || index < 0 || index >= view.count || view.component_type != 5126 || view.components != 3) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const std::uint64_t offset = view.offset + static_cast<std::uint64_t>(view.stride) * static_cast<std::uint64_t>(index);
    if (offset + 12 > view.bytes->size()) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    std::memcpy(out, view.bytes->data() + offset, 12);
    if (!std::isfinite(out[0]) || !std::isfinite(out[1]) || !std::isfinite(out[2])) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    return true;
}

bool ReadIndex(const AccessorView& view, int index, std::uint32_t& out, std::string& error) {
    if (view.bytes == nullptr || index < 0 || index >= view.count || view.components != 1) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    const std::uint64_t offset = view.offset + static_cast<std::uint64_t>(view.stride) * static_cast<std::uint64_t>(index);
    const unsigned char* data = view.bytes->data();
    if (view.component_type == 5121) {
        if (offset + 1 > view.bytes->size()) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
        out = data[offset];
        return true;
    }
    if (view.component_type == 5123) {
        if (offset + 2 > view.bytes->size()) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
        out = static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8);
        return true;
    }
    if (view.component_type == 5125) {
        if (offset + 4 > view.bytes->size()) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
        out = static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
              (static_cast<std::uint32_t>(data[offset + 2]) << 16) | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
        return true;
    }
    Fail(error, "The character mesh uses an unsupported feature.");
    return false;
}

bool InsideBounds(float x, float y, float z) {
    return x >= kCharacterMeshMinX - kBoundSlack && x <= kCharacterMeshMaxX + kBoundSlack &&
           y >= kCharacterMeshMinY - kBoundSlack && y <= kCharacterMeshMaxY + kBoundSlack &&
           z >= kCharacterMeshMinZ - kBoundSlack && z <= kCharacterMeshMaxZ + kBoundSlack;
}

bool LoadBuffers(const std::filesystem::path& gltf_path, const Json& root, std::vector<std::vector<unsigned char>>& buffers, std::string& error) {
    const Json* listed = root.Find("buffers");
    if (listed == nullptr || listed->type != JsonType::Array || listed->array.empty()) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    buffers.resize(listed->array.size());
    for (std::size_t index = 0; index < listed->array.size(); ++index) {
        const Json& buffer = listed->array[index];
        std::string uri;
        int byte_length = 0;
        if (buffer.type != JsonType::Object || !AsString(buffer.Find("uri"), uri) || !AsInt(buffer.Find("byteLength"), byte_length) ||
            byte_length < 0 || static_cast<std::size_t>(byte_length) > kMaxBufferBytes || !SafeBufferName(uri)) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
        std::vector<unsigned char> file;
        if (!ReadFileBytes(gltf_path.parent_path() / uri, kMaxBufferBytes, file, error)) {
            return false;
        }
        if (file.size() < static_cast<std::size_t>(byte_length)) {
            Fail(error, "Could not read the character mesh.");
            return false;
        }
        file.resize(static_cast<std::size_t>(byte_length));
        buffers[index] = std::move(file);
    }
    return true;
}

}  // namespace

bool LoadCharacterMesh(const std::filesystem::path& gltf_path, LoadedMesh& mesh, std::string& error) {
    mesh = {};
    error.clear();
    static_assert(std::endian::native == std::endian::little, "glTF buffers are little-endian");

    std::vector<unsigned char> json_bytes;
    if (!ReadFileBytes(gltf_path, kMaxJsonBytes, json_bytes, error)) {
        return false;
    }
    std::size_t start = 0;
    if (json_bytes.size() >= 3 && json_bytes[0] == 0xEF && json_bytes[1] == 0xBB && json_bytes[2] == 0xBF) {
        start = 3;
    }
    const std::string text =
        json_bytes.size() <= start
            ? std::string()
            : std::string(reinterpret_cast<const char*>(json_bytes.data() + start), json_bytes.size() - start);
    Json root;
    if (!ParseJson(text, root)) {
        Fail(error, "Could not read the character mesh.");
        return false;
    }
    const Json* asset = root.Find("asset");
    std::string version;
    if (asset == nullptr || !AsString(asset->Find("version"), version) || version != "2.0") {
        Fail(error, "The character mesh is not glTF 2.0.");
        return false;
    }
    if (const Json* required = root.Find("extensionsRequired");
        required != nullptr && !(required->type == JsonType::Array && required->array.empty())) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }

    std::vector<std::vector<unsigned char>> buffers;
    if (!LoadBuffers(gltf_path, root, buffers, error)) {
        return false;
    }

    const Json* meshes = root.Find("meshes");
    if (meshes == nullptr || meshes->type != JsonType::Array || meshes->array.empty() || meshes->array[0].type != JsonType::Object) {
        Fail(error, "The character mesh has no triangles.");
        return false;
    }
    const Json* primitives = meshes->array[0].Find("primitives");
    if (primitives == nullptr || primitives->type != JsonType::Array || primitives->array.empty() ||
        primitives->array[0].type != JsonType::Object) {
        Fail(error, "The character mesh has no triangles.");
        return false;
    }
    const Json& primitive = primitives->array[0];
    if (const Json* mode = primitive.Find("mode"); mode != nullptr) {
        int mode_value = 0;
        if (!AsInt(mode, mode_value) || mode_value != 4) {
            Fail(error, "The character mesh uses an unsupported feature.");
            return false;
        }
    }
    const Json* attributes = primitive.Find("attributes");
    if (attributes == nullptr || attributes->type != JsonType::Object) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }
    int position_index = 0;
    int normal_index = 0;
    int index_index = 0;
    if (!AsInt(attributes->Find("POSITION"), position_index) || !AsInt(attributes->Find("NORMAL"), normal_index) ||
        !AsInt(primitive.Find("indices"), index_index)) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }

    AccessorView positions;
    AccessorView normals;
    AccessorView indices;
    if (!ResolveAccessor(root, buffers, position_index, positions, error) ||
        !ResolveAccessor(root, buffers, normal_index, normals, error) ||
        !ResolveAccessor(root, buffers, index_index, indices, error)) {
        return false;
    }
    if (positions.count <= 0 || positions.count > kCharacterMeshMaxVertices || positions.count != normals.count ||
        positions.component_type != 5126 || normals.component_type != 5126 || positions.components != 3 ||
        normals.components != 3 || indices.components != 1 || indices.count < 3 || indices.count % 3 != 0) {
        Fail(error, "The character mesh uses an unsupported feature.");
        return false;
    }

    mesh.positions.resize(static_cast<std::size_t>(positions.count) * 3);
    mesh.normals.resize(static_cast<std::size_t>(normals.count) * 3);
    mesh.indices.resize(static_cast<std::size_t>(indices.count));
    for (int index = 0; index < positions.count; ++index) {
        float position[3]{};
        float normal[3]{};
        if (!ReadFloat3(positions, index, position, error) || !ReadFloat3(normals, index, normal, error)) {
            mesh = {};
            return false;
        }
        // glTF right is -X. This engine's right is +X. Forward stays +Z.
        position[0] = -position[0];
        normal[0] = -normal[0];
        if (!InsideBounds(position[0], position[1], position[2])) {
            Fail(error, "The character mesh is outside the allowed bounds.");
            mesh = {};
            return false;
        }
        mesh.positions[static_cast<std::size_t>(index) * 3] = position[0];
        mesh.positions[static_cast<std::size_t>(index) * 3 + 1] = position[1];
        mesh.positions[static_cast<std::size_t>(index) * 3 + 2] = position[2];
        mesh.normals[static_cast<std::size_t>(index) * 3] = normal[0];
        mesh.normals[static_cast<std::size_t>(index) * 3 + 1] = normal[1];
        mesh.normals[static_cast<std::size_t>(index) * 3 + 2] = normal[2];
    }
    for (int index = 0; index < indices.count; ++index) {
        std::uint32_t vertex = 0;
        if (!ReadIndex(indices, index, vertex, error) || vertex >= static_cast<std::uint32_t>(positions.count)) {
            Fail(error, "The character mesh uses an unsupported feature.");
            mesh = {};
            return false;
        }
        mesh.indices[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(vertex);
    }
    return true;
}
