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

All dependencies are git submodules under `ext/` (except imnodes, which is vendored directly).

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

**Requirements:** Clang, CMake 3.24+, `glslc` (from the Vulkan SDK), GNU Make.

```bash
git clone --recurse-submodules <repo-url>
cd vkit
make build
```

`make build` compiles shaders with `glslc`, configures CMake, then builds the project.

To run after building:

```bash
make run
# or directly:
./build/vkit
```

To recompile shaders only:

```bash
make glslc
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
│   ├── material/      # Per-material fragment shaders
│   ├── operators/     # Compute operator shaders
│   ├── procedural/    # Procedural generator compute shaders
│   └── ibl/           # IBL precomputation compute shaders
├── assets/            # Runtime assets (compiled shaders, models, fonts, envmaps)
├── ext/               # Third-party libraries (git submodules)
└── Makefile
```
