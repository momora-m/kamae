cmake_minimum_required(VERSION 3.24)

project(editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
  message(FATAL_ERROR
    "editor は Win32 と DirectX 11 専用です。Visual Studio 2022 と Windows 10 SDK がある Windows で構成してください。")
endif()

include(FetchContent)

# Dear ImGui の docking タグ。ルートに CMakeLists.txt がないため、
# FetchContent_MakeAvailable ではなくソースだけ取得する。
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9b-docking
  GIT_SHALLOW TRUE
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

add_executable(editor WIN32
  src/main.cpp
  src/renderer.cpp
  src/renderer.hpp
  src/walk.cpp
  src/walk.hpp
  src/actions.cpp
  src/actions.hpp
  src/overlap.cpp
  src/overlap.hpp
  src/attack.cpp
  src/attack.hpp
  src/attack_mark.cpp
  src/attack_mark.hpp
  src/approach.cpp
  src/approach.hpp
  src/subject_panel.cpp
  src/subject_panel.hpp
  src/roster.cpp
  src/roster.hpp
  src/arena_file.cpp
  src/arena_file.hpp
  src/trial.cpp
  src/trial.hpp
  shaders/lit.hlsl
  ${IMGUI_SOURCES}
)

set_source_files_properties(shaders/lit.hlsl PROPERTIES
  HEADER_FILE_ONLY TRUE
  VS_TOOL_OVERRIDE "None"
)

target_include_directories(editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(editor PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

target_link_libraries(editor PRIVATE
  d3d11
  dxgi
  d3dcompiler
)

if(MSVC)
  target_compile_options(editor PRIVATE /W4 /permissive- /utf-8)
endif()

add_custom_command(TARGET editor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/shaders/lit.hlsl"
          "$<TARGET_FILE_DIR:editor>/lit.hlsl"
  VERBATIM
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT editor)
set_property(TARGET editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:editor>")
