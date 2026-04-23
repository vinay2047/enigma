# Escape Room — 3D OpenGL C++ Game
# akan deka
## Build Instructions

### Prerequisites
- **CMake** 3.16+ ([cmake.org](https://cmake.org/download/))
- **Git** (for FetchContent to download GLFW, GLAD, GLM automatically)
- **Compiler**: MinGW-w64 (GCC 11+) **or** MSVC 2019+

> All dependencies (GLFW, GLAD, GLM) are downloaded automatically by CMake — no manual installs needed.

---

### Build Steps (Windows)

```bash
cd C:\Users\akan\OneDrive\Documents\Labs\EscapeRoom

# Configure (downloads deps on first run — needs internet)
cmake -B build -G "MinGW Makefiles"
# OR for MSVC:
cmake -B build

# Build
cmake --build build --config Release

# Run
build\EscapeRoom.exe
```

---

## Controls

| Key | Action |
|-----|--------|
| `WASD` | Move |
| `Mouse` | Look around |
| `Space` | Jump |
| `Ctrl` | Crouch |
| `E` | Interact with object |
| `H` | Show hint for nearest puzzle |
| `Esc` | Pause menu |
| `S` (paused) | Save game |
| `L` (paused/menu) | Load save |
| `Q/E` | Rotate mirror (when at mirror puzzle) |
| `R` | Restart (on win/lose screen) |
| `Enter` | Start game (from main menu) |

---

## Puzzle Walkthrough

| # | Room | Puzzle | Solution |
|---|------|--------|---------|
| 1 | Entry Hall | Hidden button | Press `E` on the painting, then the hidden button |
| 2 | Entry Hall | Brass key lock | Pick up key, use on door |
| 3 | Study Room | Light sequence | Watch 4 lights, press same order: 1→3→2→4 |
| 4 | Study Room | Lever sequence | Left → Right → Middle (book clue) |
| 5 | Study Room | Pressure plate | Step on the plate that rises |
| 6 | Study Room | Code pad | Enter `4821` (from code note) |
| 7 | The Vault | Mirror light | Press `E` on mirror, rotate with `Q/E` until sensor lights green |

---

## Project Structure

```
EscapeRoom/
├── main.cpp              ← Game loop, state machine, input
├── CMakeLists.txt        ← Build config (auto-downloads deps)
├── src/
│   ├── shader.h/.cpp     ← GLSL shader loader
│   ├── player.h/.cpp     ← First-person camera, WASD, jump, crouch
│   ├── collision.h/.cpp  ← AABB + ray-cast + slide resolution
│   ├── lighting.h/.cpp   ← Phong point lights with flicker
│   ├── inventory.h/.cpp  ← Item pickup and HUD display
│   ├── puzzle.h/.cpp     ← 7 puzzle types and state machine
│   ├── room.h/.cpp       ← 3 rooms with procedural textures
│   ├── renderer.h/.cpp   ← OpenGL draw calls, box mesh, 2D quads
│   ├── particle.h/.cpp   ← CPU particle system (dust/fog)
│   ├── ui.h/.cpp         ← Full bitmap-font HUD (no freetype needed)
│   └── saveload.h/.cpp   ← Binary save/load
└── shaders/
    ├── main.vert/frag    ← Phong + fog + 8 point lights
    ├── shadow.vert/frag  ← Depth pass
    ├── particle.vert/frag← Billboard particles
    └── ui.vert/frag      ← 2D orthographic HUD
```
