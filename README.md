# Auschwitz I Educational 3D Reconstruction

An interactive, high-fidelity 3D educational reconstruction of the Auschwitz I concentration camp. Developed using modern C++17 and custom OpenGL rendering, this engine emphasizes structural accuracy, performant flyweight-instanced batching, and procedural generation for environmental realism.

## 🏗 System Architecture

The application is built on a highly modular Zone-based render pipeline. Each core zone or feature of the reconstruction is strictly isolated into discrete components.

### 🔌 Core Rendering Features
- **Lighting & Illumination Engine**: Native multi-light architecture (Phong shading model) scaling up to 32 Point Lights, 36 Spot Lights, and Directional ambient sources (sun/moon) compiled directly within `phong.frag`.
- **Flyweight Instancing**: Aggressive re-use of standard OpenGL primitive meshes (`Cube`, `Cylinder`, `Plane`, `Sphere`). Identical buildings draw off heavily localized transform matrices (VBO reuse), rendering a vast campus of 28 barracks effortlessly.
- **Procedural Geometry**:
  - **L-System Fractals**: Recursively constructed 3D boundary trees (`LSystemTree`), utilizing L-System expansions. 
  - **Bezier Curves**: Curved metal arches (e.g. *ARBEIT MACHT FREI* gate signage) and sagging wire fences generated via calculated Cubic Bezier matrices (`BezierCurve.h`).
  - **Ruled Surfaces**: Specialized procedural meshing for smooth transition models.
- **Dynamic Environments**: 
  - Real-time interpolated Day/Night Cycle simulating accurate twilight offsets.
  - Procedural panoramic horizon mapping mapped across cylindrical geometries.
  - Dynamically generated fallback textures (4x4 static noise generation) for error-proof loading stability.

---

## 📂 File & Directory Structure

```text
/
├── .vscode/               # IDE configuration properties
├── extern/                # External dependencies (GLAD, GLFW, GLM, STB_Image)
├── shaders/
│   ├── phong.vert         # Primary Vertex shader math
│   ├── phong.frag         # Primary Fragment illumination
│   ├── unlit.vert         # Emissive glowing body Vert
│   ├── unlit.frag         # Emissive glowing body Frag
│   ├── alpha.vert         # Transparency sorting Vert
│   └── alpha.frag         # Transparency sorting Frag
├── src/
│   ├── main.cpp           # App entry, glad initiation, global states, rendering loop
│   ├── Scene.h            # Central orchestrator. Manages initialization, lighting arrays, textures
│   ├── Camera.h           # Free-flight 3D spectator camera matrix control
│   ├── Texture.h          # STBI Wrapper 
│   ├── Shader.h           # OpenGL Shader compilation/linking class
│   ├── LSystemTree.h      # Recursive geometry generator for landscape trees
│   ├── ParticleSystem.h   # Dynamic snow/dust emission and tracking
│   ├── HorizonSystem.h    # Boundary panoramas and volumetric skybox abstraction
│   ├── DayNightCycle.h    # Sun directional vector math via DeltaTime
│   │
│   ├── primitives/        # Geometry Primitive Engine definitions
│   │   ├── Cube.h, Cylinder.h, Plane.h, Sphere.h, RuledSurface.h, BezierTube.h
│   │
│   └── zones/             # Modulized Sector Geography
│       ├── EntranceGate.h   # Contains front wall, Gatehouse, ARBEIT Bezier sign
│       ├── FenceSystem.h    # Barbed wire procedural sag networks and concrete posts
│       ├── BarrackGrid.h    # The primary 28 prisoner housing blocks with roof layout
│       ├── GuardTowers.h    # Placements for perimeter overwatch architecture
│       ├── GroundZone.h     # Primary dirt mesh, Appellplatz roll-call square, gravel
│       ├── StreetLamps.h    # Procedural light-posts emitting spot/point lights
|       ├── BarrackInteriors.h
|       ├── Block11Zone.h
|       ├── CrematoryZone.h
|       └── AdminZone.h
└── textures/              # Diffuse and Alpha texture assets
```

## 🛠 Compilation & Setup Instructions

### Environment Prerequisites
- Windows OS (Visual Studio MSVC or MinGW)
- **C++17** compiler or higher.
- OpenGL 3.3 Core capability.

### Visual Studio Integration
The project directories are heavily structured for direct compilation via Visual Studio. 
Ensure the `extern/` directory holds active implementations of:
- **GLFW3** (`glfw3.lib`)
- **GLAD**
- **GLM** (headers only)
- **STB_Image** (Image ingestion)

**Include Directories:** Add `/extern`, `/extern/glad`, `/extern/glm`, and `/src`  
**Library Directories:** Add paths bridging out to your local version of `glfw3.lib`. 
**Linker Settings:** Target `opengl32.lib` and `glfw3.lib`.

### Controls
Once compiled and launched:
- **W, A, S, D:** Free-flight navigation.
- **Shift:** Hold to increase velocity (sprint).
- **Q / E:** Decelerate / Escalate vertical `Y` height natively.
- **Mouse Movement:** Panning camera system.
- **Mouse Scroll:** Dynamic FOV.

## 📝 Recent Implementation Changes
- **Grid De-Coupling**: Reduced dense overlapping Barrack scale down to tightly tracked `32m` blocks, perfectly separating them by `6m` along symmetrical avenues.
- **Interior Viewing**: Disabled `GL_CULL_FACE` explicitly to render interior wall spaces for user exploration through the entrance doors.
- **Geometry Z-Fighting Fix**: Hand-calibrated the texture coordinates of dirt-roads, foundations, and glass windows (shifted outwards `0.04m`) to prevent OpenGL's co-planar tearing artifacts.
- **Roof Pitch Correction**: Fixed matrix alignments where the sloped gable panels were improperly rotated, strictly bolting them into the 6-meter edge alignment.
