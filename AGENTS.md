# AGENTS.md — Auschwitz I 3D Reconstruction

## Build (Visual Studio / MSBuild)

**Prerequisites:**
- Visual Studio 2022+ with C++17 support (PlatformToolset v145)
- OpenGL 3.3 Core capable GPU
- `extern/` directory must exist with:
  - `glfw3.lib` in `extern/lib/`
  - GLAD, GLM, STB_Image headers in `extern/`
  - *(Note: `extern/` is gitignored but required for build)*

**Compile:**
```powershell
# Open solution in Visual Studio and build, or use MSBuild:
msbuild Auschwitz_tour.vcxproj /p:Configuration=Debug /p:Platform=x64
```

**Output:** `x64/Debug/Auschwitz_tour.exe` (console application)

## Entry Points & Architecture

- **Entry:** `src/main.cpp` — GLFW init, render loop, 7 render passes
- **Scene orchestrator:** `Scene.h` — manages all zones, lights, textures
- **Zone modules:** `src/zones/*.h` — each camp sector (BarrackGrid, EntranceGate, etc.)
- **Primitives:** `src/primitives/*.h` — Flyweight-instanced meshes (Cube, Cylinder, etc.)
- **Shaders:** `shaders/*.vert`, `shaders/*.frag` — loaded at runtime from working directory

## Render Pipeline (7 Passes, in order)

1. Clear + Skybox
2. Stars (unlit)
3. Opaque geometry (Phong)
4. Celestial bodies (sun/moon, unlit)
5. Alpha/transparency (sorted)
6. Particles (snow/dust)
7. HUD (orthographic overlay)

## Key Runtime Behavior

- **Working directory matters:** Shaders and textures loaded via relative paths (`shaders/`, `textures/`)
- **GL_CULL_FACE disabled** — Required for interior wall rendering (allows viewing inside barracks)
- **Delta time capped at 0.1s** — Prevents physics explosion on lag spikes
- **MSAA 4x enabled** — `GLFW_SAMPLES` set in main.cpp

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Shift | Sprint |
| Space / Ctrl | Up / Down |
| Mouse | Look |
| Scroll | FOV zoom |
| T | Cycle time speed (pause/1x/8x/30x) |
| H | Toggle HUD |
| C | Toggle cursor lock |
| F | Toggle fullscreen |
| ESC | Quit |

## Adding New Zones

1. Create `src/zones/YourZone.h`
2. Add `#include` and member to `Scene.h`
3. Call `init()` in `Scene::init()`, `render()` in `Scene::renderOpaque()`
4. Add header to `.vcxproj` `<ClInclude>` list

## Shader Editing

Shaders are loaded at runtime from `shaders/` directory. No recompilation needed for shader changes—just restart the executable.

## Git Constraints

- `extern/` is gitignored but required locally
- `old/` directory contains legacy code—do not reference in new code
