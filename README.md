# vkit — Vulkan Renderer

A Vulkan-based PBR renderer with a node-based material graph editor.

---

## Proposal: Shadows & Ray Tracing

This document outlines three incremental approaches to adding shadow and ray tracing support, ordered by complexity.

---

### Option 1 — Shadow Maps (Medium, ~1 week)

Classic rasterized shadows using a depth-only pass from the light's point of view.

**What needs to be added:**

| Area | Change |
|------|--------|
| Light system | `LightComponent` / `LightManager` — store position, direction, projection matrix |
| Shadow pass | New `RenderPass` (depth-only) rendering scene geometry from light POV |
| Shadow sampler | `vk::Sampler` with compare op (`eLessOrEqual`) for PCF |
| Shadow map descriptor | Bind shadow depth image to PBR fragment shader |
| Shader changes | Sample shadow map in `principled_bsdf.glsl`, compute shadow factor, multiply into diffuse/specular |
| App wiring | Run shadow pass before main pass each frame, pass light matrix as push constant / UBO |

**Approximate scope:** ~6–8 new files, ~4 modified files.

**Key constraints:**
- One shadow map = one directional/spot light. Point lights require a cube shadow map (6 faces).
- PCF (percentage-closer filtering) is needed to avoid hard aliasing — requires `sampler2DShadow` + `textureProj`.
- Cascaded shadow maps (CSM) are needed for large scenes; single-cascade is fine for a first pass.

---

### Option 2 — Ray Queries (Medium-Hard, ~2 weeks)

Hybrid approach: keep the rasterization pipeline, but add hardware ray tracing for shadow/occlusion queries inside the fragment shader via `GL_EXT_ray_query`.

**What needs to be added:**

| Area | Change |
|------|--------|
| BLAS (bottom-level AS) | Build one `VkAccelerationStructureKHR` per unique mesh geometry |
| TLAS (top-level AS) | Build one TLAS per frame from all mesh instances (transform matrices) |
| AS builder | `AccelerationStructureBuilder` — handles scratch buffers, `vkCmdBuildAccelerationStructuresKHR`, compaction |
| Device features | Enable `VkPhysicalDeviceRayQueryFeaturesKHR`, `VkPhysicalDeviceAccelerationStructureFeaturesKHR` |
| Descriptor | Bind TLAS as `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` in the PBR layout |
| Shader changes | `#extension GL_EXT_ray_query : enable`; replace shadow map sample with `rayQueryEXT` trace against TLAS |
| TLAS rebuild | Every frame (or on transform change) — update TLAS instance buffer and refit/rebuild |

**Approximate scope:** ~8–12 new files, ~5 modified files.

**Key constraints:**
- Requires `VK_KHR_acceleration_structure` + `VK_KHR_ray_query` device extensions.
- BLAS only needs rebuilding when mesh geometry changes (rare). TLAS needs refit every frame if objects move.
- Soft shadows via ray queries require multiple rays per pixel (noise) — needs temporal accumulation or a blue-noise sample pattern.
- This gives correct self-shadowing and ambient occlusion "for free" once the TLAS is in place.

---

### Option 3 — Full Ray Tracing Pipeline (Hard, ~3–4 weeks)

Replace the fragment shading stage with a dedicated RT pipeline using `VK_KHR_ray_tracing_pipeline`. Enables global illumination, reflections, refractions.

**What needs to be added:**

| Area | Change |
|------|--------|
| Everything in Option 2 | BLAS/TLAS builder, device features |
| RT pipeline | `vkCreateRayTracingPipelinesKHR` with shader groups |
| Shader binding table (SBT) | Buffer with raygen / miss / hit group records, `vkCmdTraceRaysKHR` dispatch |
| Shader stages | `.rgen` (ray generation), `.rchit` (closest hit), `.rahit` (any hit / alpha), `.rmiss` (miss / sky) |
| Material access | All meshes' vertex/index/material data must be accessible by address — `VK_KHR_buffer_device_address`, bindless material buffer |
| Output image | Storage image (`VK_IMAGE_USAGE_STORAGE_BIT`) written by `.rgen`, then blitted/composited to swapchain |
| Denoising (optional) | Temporal accumulation pass or integration with DLSS/XeSS/OIDN |
| App restructure | RT pass replaces or augments the raster pass; may keep raster for primary visibility + RT for secondary rays |

**Approximate scope:** ~15–20 new files, ~8 modified files.

**Key constraints:**
- Requires `VK_KHR_ray_tracing_pipeline` on top of the acceleration structure extensions.
- The SBT layout is finicky — each hit group record must align to `shaderGroupHandleAlignment`.
- Bindless material access (device addresses or a large descriptor array) is mandatory because the hit shader can land on any primitive.
- Full path tracing at interactive framerates requires either a very low sample count + denoising, or progressive accumulation when the camera is still.

---

### Effort Summary

| Approach | Difficulty | Estimated Time | New Files | Modified Files | Result |
|----------|-----------|----------------|-----------|----------------|--------|
| Shadow Maps | Medium | ~1 week | 6–8 | ~4 | Hard shadows from one light |
| Ray Queries | Medium-Hard | ~2 weeks | 8–12 | ~5 | Correct soft shadows + AO, hybrid pipeline |
| Full RT Pipeline | Hard | ~3–4 weeks | 15–20 | ~8 | Reflections, GI, refractions |

**Recommended path:** Shadow Maps → Ray Queries. The BLAS/TLAS infrastructure built for ray queries also unlocks the full RT pipeline later with no wasted work.
