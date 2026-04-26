cmake_minimum_required(VERSION 3.16)
project(EscapeRoom VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow newer CMake versions (4.x) to build older dependencies like GLAD
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

# ── GLFW ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLAD ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    set(GLAD_PROFILE  "core"  CACHE STRING "" FORCE)
    set(GLAD_API      "gl=3.3" CACHE STRING "" FORCE)
    set(GLAD_GENERATOR "c"    CACHE STRING "" FORCE)
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

# ── GLM ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    main.cpp
    src/shader.cpp
    src/player.cpp
    src/collision.cpp
    src/lighting.cpp
    src/inventory.cpp
    src/puzzle.cpp
    src/room.cpp
    src/renderer.cpp
    src/particle.cpp
    src/ui.cpp
    src/saveload.cpp
)

add_executable(EscapeRoom ${SOURCES})

target_include_directories(EscapeRoom PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(EscapeRoom PRIVATE glfw glad glm)

if(WIN32)
    target_compile_definitions(EscapeRoom PRIVATE PLATFORM_WINDOWS _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(EscapeRoom PRIVATE opengl32)
endif()

# Copy runtime assets
add_custom_command(TARGET EscapeRoom POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:EscapeRoom>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:EscapeRoom>/assets
)
