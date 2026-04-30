# Enigma — A 3D First-Person Escape Room Game



A fully procedural 3D escape room game built from scratch in **C++17** and **OpenGL 3.3**, with no external asset files — every texture, font glyph, and visual element is generated at runtime. The player must navigate three interconnected rooms, solving 10 randomized puzzles under a 10-minute countdown to escape.

---

## Table of Contents

- [Overview](#overview)
- [Gameplay](#gameplay)
- [Architecture](#architecture)
- [Rendering Pipeline](#rendering-pipeline)
- [Puzzle System](#puzzle-system)
- [Physics & Collision](#physics--collision)
- [Procedural Content Generation](#procedural-content-generation)
- [Game State Machine](#game-state-machine)
- [Controls](#controls)
- [Build Instructions](#build-instructions)
- [Project Structure](#project-structure)

---

## Overview

**Enigma** is a single-player, first-person puzzle game where the player is trapped inside a mysterious building and must solve a chain of environmental puzzles to find a way out before time expires. The entire game runs on a custom-built engine with:

- **Zero external assets** — all textures are procedurally generated via hash-based noise functions, all fonts are rendered through a built-in 5×7 bitmap character set, and all geometry is constructed from axis-aligned box primitives.
- **Blinn-Phong shading** with up to 8 dynamic point lights, per-object shininess, and exponential distance fog.
- **Randomized puzzles** — key codes, lever orders, and button sequences are shuffled each playthrough using a time-seeded Mersenne Twister, so no two runs are the same.
- **Full game loop** — menu → gameplay → pause → win/lose, with binary save/load support.

### Dependencies (auto-downloaded)

| Library | Purpose | Version |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | Windowing, input, OpenGL context | 3.4 |
| [GLAD](https://github.com/Dav1dde/glad) | OpenGL 3.3 Core function loader | 0.1.36 |
| [GLM](https://github.com/g-truc/glm) | Mathematics (vectors, matrices, transforms) | 0.9.9.8 |

All three are fetched automatically by CMake's `FetchContent` — no manual installs required.

---

## Gameplay

The player wakes up in the **Entry Hall** of a locked building. A countdown of **10 minutes** (600 seconds) ticks in the HUD. To escape, the player must:

1. **Entry Hall** — Discover a hidden button behind a painting → retrieve a brass key from a secret drawer → unlock the door to the next room.
2. **Study Room** — Memorize and repeat a randomized 4-light sequence → find a book clue → pull three levers in the correct shuffled order → activate a pressure plate → enter a randomized 4-digit code on a keypad → advance to the vault.
3. **The Vault** — Read a compass riddle on a wall tablet → press four directional wall buttons in the shuffled order → unlock a chest with an iron key → place a crimson crystal on an altar to fire a laser beam → rotate a mirror to reflect the beam onto a photo sensor → escape.

If the timer reaches zero, the player loses. On win or lose, pressing **R** fully restarts the game with newly randomized puzzle parameters.

---

## Architecture

The engine is organized into 11 modular source files, each owning a distinct subsystem:

```
┌──────────────────────────────────────────────────────────────┐
│                        main.cpp                              │
│  Game loop · State machine · Input dispatch · Puzzle setup   │
├──────────────┬───────────────┬───────────────┬───────────────┤
│   Player     │   Renderer    │      UI       │   Particles   │
│  (camera,    │ (draw calls,  │ (bitmap font, │ (CPU billboard│
│   movement,  │  box mesh,    │  HUD, menus,  │  dust / fog   │
│   jump/      │  shadow pass) │  code pad)    │  effects)     │
│   crouch)    │               │               │               │
├──────────────┼───────────────┼───────────────┼───────────────┤
│  Collision   │   Lighting    │   Inventory   │   Save/Load   │
│ (AABB, ray-  │ (point lights,│ (item pickup, │ (binary file  │
│  cast, slide)│  flicker)     │  key/crystal) │  persistence) │
├──────────────┴───────────────┴───────────────┴───────────────┤
│               Room / Scene Manager                           │
│  3 rooms · Procedural textures · GameObjects · Transforms    │
├──────────────────────────────────────────────────────────────┤
│                     Puzzle Manager                            │
│  10 puzzles · 7 types · Randomized · Callback-driven         │
└──────────────────────────────────────────────────────────────┘
```

### Key design decisions

- **ECS-lite**: Each `GameObject` holds its own transform, AABB, material, interaction type, and door animation state. This flat structure avoids the overhead of a full ECS while keeping objects self-describing.
- **Callback-driven puzzles**: Each `Puzzle` stores a `std::function<void()> onSolve` lambda that directly mutates the scene (opening doors, revealing items, changing lighting). This avoids a centralized event bus while keeping puzzle effects co-located with puzzle definitions.
- **No texture loading**: The `assets/` folder was intentionally left unused — all visual surfaces are generated by mathematical noise and pattern functions, meaning the game has no external file dependencies beyond its GLSL shaders.

---

## Rendering Pipeline

Rendering is handled by `Renderer` and four GLSL shader programs:

### Shaders

| Shader | File | Purpose |
|--------|------|---------|
| **Main** | `main.vert` / `main.frag` | Blinn-Phong illumination with up to 8 point lights, per-object shininess, procedural textures, and exponential fog |
| **Shadow** | `shadow.vert` / `shadow.frag` | Depth-only pass (for future shadow mapping support) |
| **Particle** | `particle.vert` / `particle.frag` | Camera-facing billboard quads with per-particle alpha and color |
| **UI** | `ui.vert` / `ui.frag` | 2D orthographic projection for HUD elements |

### Lighting model

The fragment shader implements **Blinn-Phong** reflection:

```
result = ambient + Σ (diffuse + specular) × attenuation
```

- **Ambient**: `ambientStrength × ambientColor × baseColor` — per-room configurable.
- **Diffuse**: `max(dot(N, L), 0) × lightColor × baseColor × atten`.
- **Specular**: `pow(max(dot(N, H), 0), shininess) × lightColor × 0.25 × atten` — half-vector model.
- **Attenuation**: `intensity / (1 + 0.22d + 0.20d²) × max(0, 1 − d/radius)` — physically-motivated quadratic falloff with a hard radius cutoff.
- **Fog**: `mix(fogColor, litColor, exp(−fogDensity × 0.1 × dist))` clamped to [0.6, 1.0].
- **Brightness floor**: `max(result, baseColor × 0.35)` prevents pitch-black areas.

### Geometry

All 3D objects are drawn using a single **unit cube mesh** (24 vertices, 36 indices) scaled by each `GameObject`'s transform. The `Renderer::buildBox()` method generates the VAO/VBO/EBO once at startup. 2D UI elements use a separate unit quad mesh.

---

## Puzzle System

The `PuzzleManager` supports **7 distinct puzzle types**, instantiated into **10 puzzle instances** across the three rooms:

| Type | Mechanic | Randomized? |
|------|----------|-------------|
| `HIDDEN_BUTTON` | Find and press a concealed button | No (spatial discovery) |
| `KEY_LOCK` | Use a specific key item on a door/chest | No (item gate) |
| `LIGHT_SEQUENCE` | Watch 4 lights flash, repeat the order | ✅ Shuffled each run |
| `CODE_PAD` | Enter a 4-digit numeric code | ✅ Random 1000–9999 |
| `LEVER_SEQUENCE` | Pull 3 levers in the correct order | ✅ Shuffled each run |
| `PRESSURE_PLATE` | Step on floor plates in order | No (single plate) |
| `MIRROR_LIGHT` | Rotate a mirror to aim a reflected laser beam at a sensor | No (fixed angle) |

### Puzzle chaining

Puzzles are **sequentially gated** — solving one reveals the next:

```
Painting → Hidden Button → Drawer/Key → Door Lock
         ↓
Light Sequence → Code Note → Levers (book clue) → Pressure Plate → Code Pad → Vault Door
         ↓
Compass Riddle → Wall Buttons → Chest/Key → Crystal → Altar → Laser → Mirror → Sensor → Exit
```

Each puzzle's `onSolve` callback mutates the scene in real time — toggling object visibility, shifting positions, updating interaction types, and spawning particle effects.

---

## Physics & Collision

### AABB collision

Every solid `GameObject` has an axis-aligned bounding box (`AABB`) computed from its transform center and half-extents. The player is modeled as a vertical capsule approximated by an AABB (0.4m wide × 1.8m tall).

Each frame, `resolveCollision()` performs **iterative slide resolution** — projecting the player's movement vector against the nearest penetrating face and sliding along the collision surface, preventing tunneling through walls and furniture.

### Ray-cast interaction

The `pickObject()` function fires a ray from the player's eye position along the camera's forward vector and tests intersection against every visible, interactable object's AABB using a standard **slab method** ray-AABB test. The nearest hit within `INTERACT_DIST` (5 units) is selected for interaction.

### Player physics

- **Gravity**: −9.8 m/s² applied every frame; the player is grounded at y=0.
- **Jumping**: Applies an upward impulse; arc follows ballistic trajectory until landing.
- **Crouching**: Reduces eye height from 1.7m to 0.9m and shrinks the collision extent.

---

## Procedural Content Generation

### Textures

Five procedural texture generators create all surface materials at runtime using hash-based noise:

| Texture | Resolution | Visual |
|---------|------------|--------|
| `makeStoneTexture()` | 128×128 | Gray-brown noise with coarse grain blocks |
| `makeWoodTexture()` | 128×128 | Warm brown with sinusoidal ring patterns simulating wood grain |
| `makeMetalTexture()` | 128×128 | Cool blue-gray with vertical sine banding |
| `makeBrickTexture()` | 128×64 | Red-brown blocks with mortar lines (offset alternate rows) |
| `makeCarpetTexture()` | 64×64 | Deep purple noise for fabric texture |

Each generator calls `makeProceduralTexture(w, h, fn)` which evaluates a per-pixel lambda, uploads the RGB data to an OpenGL texture, and generates mipmaps.

### Bitmap font

The UI system renders text using a built-in **5×7 pixel bitmap font** — each ASCII character is encoded as a 5-column × 7-row bit pattern. Characters are rendered as filled rectangles (one quad per "on" pixel), scaled and tinted by the caller. This eliminates any dependency on FreeType or font files.

---

## Game State Machine

The game operates as a finite state machine with 8 states:

```
         ┌──────────────────────────────────────────────────┐
         │                                                  │
   ┌─────▼─────┐    Enter    ┌──────────┐    Esc    ┌──────┴───┐
   │   MENU    │────────────▶│ PLAYING  │◀────────▶│  PAUSED  │
   └───────────┘             └────┬─────┘          └──────────┘
         ▲                        │                     │
         │                   Interact                Save/Load
         │                   with pad                   │
     R (restart)         ┌────▼──────┐                  │
         │               │ CODE_PAD  │                  │
         │               │    UI     │                  │
   ┌─────┴─────┐         └──────────┘                  │
   │   WIN     │◀── winTimer expires                    │
   └───────────┘                                        │
   ┌───────────┐                                        │
   │   LOSE    │◀── timer ≤ 0                          │
   └───────────┘                                        │

   SEQUENCE_UI: active during light sequence playback (input locked)
   INSPECT: reserved for future close-up examination UI
```

Input routing is context-sensitive — `WASD` only moves the player in `PLAYING`, digit keys only register in `CODE_PAD_UI`, and `Q/E` rotate the mirror only when puzzle 9 is active.

---

## Controls

| Key | Action |
|-----|--------|
| `W A S D` | Move forward / left / backward / right |
| `Mouse` | Look around (first-person camera) |
| `Space` | Jump |
| `Left Ctrl` | Crouch (lowers eye height, shrinks hitbox) |
| `E` | Interact with the nearest object / rotate mirror right |
| `Q` | Rotate mirror left (when at mirror puzzle) |
| `H` | Show hint for the nearest puzzle |
| `Esc` | Pause / unpause |
| `S` *(paused)* | Save game to `escape_save.dat` |
| `L` *(paused / menu)* | Load saved game |
| `0–9` *(code pad)* | Enter digits on the keypad |
| `Enter` *(code pad)* | Submit code |
| `Backspace` *(code pad)* | Delete last digit |
| `Enter` *(menu)* | Start a new game |
| `R` *(win / lose)* | Restart with fresh randomized puzzles |
| `Q` *(paused)* | Quit the game |

---

## Build Instructions

### Prerequisites

- **CMake** 3.16 or newer — [cmake.org/download](https://cmake.org/download/)
- **Git** — required for CMake `FetchContent` to clone dependencies
- **C++17 compiler**: MinGW-w64 (GCC 11+) **or** MSVC 2019+
- **Internet connection** on first build (to download GLFW, GLAD, GLM)

> All third-party libraries are downloaded and built automatically by CMake — no manual installs needed.

### Build (Windows — MinGW)

```bash
cd EscapeRoom

# Configure (first run downloads dependencies)
cmake -B build -G "MinGW Makefiles"

# Build
cmake --build build --config Release

# Run
build\EscapeRoom.exe
```

### Build (Windows — MSVC)

```bash
cd EscapeRoom

# Configure
cmake -B build

# Build
cmake --build build --config Release

# Run
build\Release\EscapeRoom.exe
```

### Build (Linux / macOS)

```bash
cd EscapeRoom
cmake -B build
cmake --build build
./build/EscapeRoom
```

> **Note:** On Linux, you may need to install windowing system dev packages: `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`

---

## Project Structure

```
EscapeRoom/
├── main.cpp                  ← Game loop, state machine, input handling, puzzle setup
├── CMakeLists.txt            ← Build configuration (auto-downloads all dependencies)
├── README.md                 ← This file
├── escape_save.dat           ← Binary save file (generated at runtime)
│
├── src/
│   ├── shader.h / .cpp       ← GLSL shader compilation and uniform utilities
│   ├── player.h / .cpp       ← First-person camera, WASD movement, jump, crouch
│   ├── collision.h / .cpp    ← AABB construction, ray-AABB intersection, slide resolution
│   ├── lighting.h / .cpp     ← Point light definition, flicker animation, shader upload
│   ├── inventory.h / .cpp    ← Item types, pickup/removal, HUD name/description lookup
│   ├── puzzle.h / .cpp       ← 7 puzzle types, state tracking, interaction handlers
│   ├── room.h / .cpp         ← Room/Scene builder, procedural textures, GameObjects
│   ├── renderer.h / .cpp     ← OpenGL draw calls, box/quad mesh, 2D rect utilities
│   ├── particle.h / .cpp     ← CPU-driven billboard particle system (dust/fog atmosphere)
│   ├── ui.h / .cpp           ← Full bitmap-font HUD: menus, code pad, hints, inventory
│   └── saveload.h / .cpp     ← Binary file save/load for game state persistence
│
└── shaders/
    ├── main.vert / .frag     ← Blinn-Phong + fog + 8 point lights + per-object shininess
    ├── shadow.vert / .frag   ← Depth-only pass (shadow map infrastructure)
    ├── particle.vert / .frag ← Camera-facing billboard particles with alpha blending
    └── ui.vert / .frag       ← 2D orthographic projection for HUD rendering
```

---

## Technical Highlights

| Feature | Details |
|---------|---------|
| **Language / Standard** | C++17 (`std::optional`, structured bindings, `if constexpr`) |
| **Graphics API** | OpenGL 3.3 Core Profile with 4× MSAA |
| **Shading** | Blinn-Phong with quadratic attenuation, radius cutoff, exponential fog |
| **Textures** | 5 procedural generators using hash-based noise (no file I/O) |
| **Lighting** | Up to 8 dynamic point lights per room with configurable flicker |
| **Collision** | AABB intersection with iterative slide-based response |
| **Interaction** | Ray-cast picking (slab method) within configurable range |
| **Particles** | CPU-simulated billboard system with per-particle lifetime, color, and alpha fade |
| **Font rendering** | Built-in 5×7 bitmap font — each glyph is a hardcoded bit pattern |
| **Puzzle RNG** | `std::mt19937` seeded with `system_clock`, shuffling sequences and generating codes |
| **Save system** | Raw binary serialization of `SaveData` struct to `escape_save.dat` |
| **Build system** | CMake 3.16+ with `FetchContent` for zero-setup dependency management |

---

*Built with OpenGL, caffeine, and an unreasonable number of procedural hash functions.*
