# vkit

A Vulkan-based PBR renderer with a node-graph material editor, built in C++23.

## Features

**Rendering**
- PBR shading — Principled BSDF, diffuse, diffuse-specular, and mix material models
- Image-based lighting (IBL) — diffuse irradiance + specular prefilter + BRDF LUT
- Transparent materials — 3-pass pipeline (depth prepass → back faces → front faces) for correct inner-surface visibility
- Opacity maps with hard discard threshold, transmissive materials with depth-prepass skip
- Shadow maps for directional / spot lights, cube shadow maps for point lights
- Inverted-hull outline pass for selected primitives
- Light gizmos rendered as view-facing billboards
- Analytical ray-sphere primitives alongside mesh geometry
- Skinned mesh animation
- glTF scene loading (via fastgltf)

**Node Graph Editor**
- GPU-executed node graph for procedural texture authoring
- **Procedural generators:** fractal noise, value noise, pattern
- **Operators:** sobel edge detect, heightmap-to-normal, normal map, tint, mix, channel remap, channel adjust
- **Material outputs:** diffuse, diffuse-specular, principled BSDF, mix material
- Live preview per-node; stale detection propagates downstream on param change
- Export / import graph to JSON

**UI**
- Dear ImGui with dockable, resizable panels
- Viewport with camera controls, primitive selection, per-primitive transform gizmos (ImGuizmo)
- Scene hierarchy, material inspector, graph editor, configuration panel

## Dependencies

All dependencies are git submodules under `ext/`.

| Library | Purpose |
|---|---|
| [glfw](https://github.com/glfw/glfw) | Window and input |
| [glm](https://github.com/g-truc/glm) | Math |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | Vulkan C++ bindings |
| [VulkanMemoryAllocator-Hpp](https://github.com/YaaZ/VulkanMemoryAllocator-Hpp) | GPU memory allocation |
| [imgui](https://github.com/ocornut/imgui) (`docking` branch) | UI |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | Transform / graph gizmos |
| [imnodes](https://github.com/Nelarius/imnodes) | Node graph UI |
| [fastgltf](https://github.com/spnda/fastgltf) | glTF 2.0 loading |
| [meshoptimizer](https://github.com/zeux/meshoptimizer) | Mesh processing |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) | Native file dialogs |
| [simdjson](https://github.com/simdjson/simdjson) | JSON parsing |
| [stb](https://github.com/nothings/stb) | Image loading |

## Building

### Requirements

**Linux:** Clang, CMake 3.24+, `glslc` (Vulkan SDK), GNU Make.

**Windows cross-compilation (from Linux):** additionally `x86_64-w64-mingw32-g++` and `zip`.

```bash
git clone --recurse-submodules <repo-url>
cd vkit
```

### Linux

| Target | Description |
|---|---|
| `make debug` | Compile shaders + configure + build (debug) |
| `make release` | Compile shaders + configure + build (release, no validation layers) |
| `make run` | Build debug then run |
| `make run-release` | Build release then run |
| `make zip-debug` | Build debug + create `dist/linux/vkit-linux-debug.zip` |
| `make zip-release` | Build release + create `dist/linux/vkit-linux-release.zip` |

Binaries land in `build/linux/debug/` and `build/linux/release/`.

### Windows (cross-compile from Linux)

| Target | Description |
|---|---|
| `make debug-win` | Compile shaders + configure + build (debug) |
| `make release-win` | Compile shaders + configure + build (release) |
| `make zip-debug-win` | Build debug + create `dist/win/vkit-win-debug.zip` |
| `make zip-release-win` | Build release + create `dist/win/vkit-win-release.zip` |

Binaries land in `build/win/debug/` and `build/win/release/`.
The produced `.exe` requires only `vulkan-1.dll` at runtime (ships with GPU drivers); all other runtime libs are statically linked.

### Shaders only

```bash
make glslc
```

### Customising the bundled assets

The default model and environment map packed into zips can be overridden:

```bash
make zip-release MODEL=my_model ENV_MAP=my_env.hdr
```

## Project Structure

```
vkit/
├── src/vkit/
│   ├── renderer/      # Vulkan renderer, render passes, draw commands
│   ├── scene/         # Scene graph (nodes, meshes, lights, cameras, skins)
│   ├── material/      # Material types and manager
│   ├── workflow/      # Node graph — nodes, execution, serializer
│   ├── texture/       # Texture manager
│   ├── imgui/         # UI host, docking layout, panel windows
│   ├── graphics/      # Low-level Vulkan device, swapchain, pipelines
│   └── ...
├── shaders/           # GLSL source (vert/frag/comp)
│   ├── operators/     # Compute operator shaders
│   ├── procedural/    # Procedural generator compute shaders
│   └── ibl/           # IBL precomputation compute shaders
├── assets/            # Runtime assets (compiled shaders, models, fonts, envmaps)
├── examples/          # Example .mgraph files and textures
├── ext/               # Third-party libraries (git submodules)
├── cmake/             # Toolchain files (MinGW-w64 cross-compilation)
├── CMakePresets.json  # Configure + build presets
└── Makefile
```
