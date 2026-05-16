# Progress

## Milestone 1 — Vulkan Window + Clear ✅
- GLFW window creation, Vulkan init (instance, device, swapchain)
- Render pass, framebuffers, sync objects
- Frame loop: acquire → clear dark blue → present
- **Commit**: `6c50f58`

## Milestone 2 — Compute Pipeline ✅
- Compute shader particle simulation (gravity, damping, projection)
- Double-buffered storage buffers (ping-pong)
- Push constants for all simulation parameters
- **Commit**: `00961a5`

## Milestone 3 — Point-Sprite Rendering ✅
- Point primitives (`VK_PRIMITIVE_TOPOLOGY_POINT_LIST`) for scalability
- Fragment shader soft circles via `gl_PointCoord`
- Additive blending: `SRC_ALPHA` / `ONE` / `ADD`
- **Commit**: `00961a5`

## Milestone 4 — Simulation State (CPU) ✅
- Camera orbit: 4°/s Y-axis pan + dual-sine elevation wobble
- Singularity random walk with boundary reflection
- Dynamic particle count: `sqrt(target/dt) * count`
- Aspect ratio, camera distance 1.5, offset 0.6
- Reinit cycle removed (runs continuously)
- **Commit**: `1f19cee`

## Milestone 5 — Reinit in Compute Shader ❌ (removed)
- Removed. Particles persist indefinitely; no countdown reset.

## Milestone 6 — Singularity Sun Rendering ✅
- 7 concentric triangle-strip layers (matching original sizes)
- Sun position projected from 3D singularity with camera orbit+elevation
- Blue glow color: RGB(0.07, 0.30, 1.0)
- **Commit**: `0141e9f`

## Milestone 7 — Procedural Textures ✅
- 64×64 radial gradient for sun glow (extracted from original GL_LUMINANCE binary)
- 16×16 radial gradient for particle glow
- Texture sampler + dual descriptor sets, UNORM format (no gamma decode)
- **Commit**: `d4f33c7`

## Milestone 8 — Audio ✅
- miniaudio integration — plays glg.wav on reinit
- Graceful fallback if file missing
- **Commit**: `6b7ee0b`

## Milestone 9 — Polish ✅
- Key controls: ESC (quit), Space (pause), F (fullscreen)
- Pause freezes simulation (dt=0 in compute shader)
- Fullscreen toggle via GLFW monitor switch
- FPS counter in window title bar (updates every second)
- **Commit**: `1f19cee`

## Milestone 10 — Config File ✅
- vkg.ini parser: particle count, fullscreen mode, target FPS
- Config auto-loaded from current directory on startup
- **Commit**: `d7652dd`

## Milestone 11 — CLI Flags + FPS Tuning ✅
- `--particles N` CLI flag (overrides config)
- `--fullscreen` flag
- `--help` / `-h` usage info
- `target_fps` option drives particle count auto-adjuster
- **Commit**: `acca92f`

## Milestone 12 — Surface Recreation on Fullscreen ✅
- `recreateSwapChain` now destroys and recreates the Vulkan surface
  (needed after `glfwSetWindowMonitor` changes native window handle)
- Fixes fullscreen toggle and window resize rendering issues
- **Commit**: `34cd1b9`

## Milestone 13 — Particle Size Scaling ✅
- `point_scale` config option and `--point-scale` CLI flag
- Multiplier on particle point size (0.1–10.0, default 1.0)
- Passed through Renderer to ParticlePushConstants.pointSizeMult
- **Commit**: `667f942`

## Milestone 14 — Sun Glow Pulsing ✅
- `sunPulse` in SimState oscillates 0.75–1.25 via sin(phase * 0.5)
- All 7 sun layer alphas multiplied by sunPulse each frame
- Creates gentle breathing effect on the singularity glow
- **Commit**: `4eb6d3f`

## Milestone 15 — Hypercolor Particles ✅
- Renamed `_pad` field to `hue` in Particle struct
- Hue computed from velocity magnitude in compute shader
- Fragment shader interpolates orange→blue-white based on hue
- Added vertex attribute location 2 for hue pipeline
- **Commit**: `1ab62e8`

## Milestone 16 — Configurable Particle Colors ✅
- `hypercolor`, `hyper_intensity`, `hyper_lo_r/g/b`, `hyper_hi_r/g/b`,
  `particle_color_r/g/b` config options in vkg.ini
- Push constants pass colors to both vert+frag shaders
- Hypercolor toggle enables/disables velocity→hue remapping
- Configurable low/high velocity endpoint colors
- **Commit**: `dfa9c34`

## Milestone 17 — Hypercolor Mode Rework + Particle Count ✅
- `hypercolor` changed from bool to `hypercolor_mode` string: `"off"`, `"color"`, `"brightness"`
- `"color"`: blends lo→hi color based on velocity hue (same as before)
- `"brightness"`: boosts base color brightness by velocity (no hue shift)
- `"off"`: uses static base color only
- Merged `particle_color_*` into `hyper_lo_*` (single base color for all modes)
- Removed `staticColor` from push constants — loR/G/B serves all three modes
- `hyper_intensity` now passed to compute shader (replaces hardcoded 8.0)
- Particle count added to titlebar: `"FPS | N particles"`
- Legacy `hypercolor = true/false` ini key still parsed (maps to color/off)
- Push constant struct shrunk 76→44 bytes (vec3 padding → flat floats)
- **Commit**: `c7a7932`

## Milestone 18 — Native HDR Display Output ✅
- scRGB swapchain (`VK_FORMAT_R16G16B16A16_SFLOAT` + `VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT`)
- HDR10 PQ fallback (`VK_FORMAT_A2B10G10R10_UNORM_PACK32` + `VK_COLOR_SPACE_HDR10_ST2084_EXT`)
- HDR metadata via `VK_EXT_hdr_metadata` (BT.2020 primaries, max/min luminance)
- `VK_EXT_swapchain_colorspace` instance extension + `VK_EXT_hdr_metadata` device extension
- Config options: `hdr = true/false`, `hdr_max_luminance` (nits)
- H toggle keybind to switch HDR on/off at runtime
- SDR fallback: UNORM sRGB swapchain when HDR disabled/unavailable
- Titlebar shows "HDR on/off" status
- Screenshot/debug buffer handles half-float format (IEEE 754 h2f conversion)
- No shader changes needed (scRGB driver auto-converts linear float → display nits)
- **Commit**: `1cb13db`

## Milestone 19 — Dynamic Particle Count (No Hardcap) ✅
- Removed compile-time `MAX_PARTICLES = 32768` hard limit
- GPU buffers allocated based on actual requested particle count
- `m_maxParticles` in Compute caps `forceParticleCount` at buffer size
- Simulation `m_maxCount` replaces static `MAX_PARTICLES`
- Simulation → Compute sync each frame (adjuster changes flow through)
- Titlebar shows `compute.particleCount()` (what's actually rendered)
- CLI `--particles` no longer capped at 32768
- Tested up to 1M particles — no validation errors, no buffer overruns
- **Commit**: `0093727`

## Milestone 20 — Benchmark Mode + Sync Fix ✅
- `--benchmark [seconds]` flag: no vsync (IMMEDIATE present), timed run, prints avg FPS
- No titlebar updates, no HDR message, no key controls in benchmark mode
- Per-image render-finished semaphores (single acquire semaphore)
- Per-frame fences with ring buffer (MAX_FRAMES_IN_FLIGHT)
- Semaphores/fences properly destroyed in `cleanupSwapChain()`
- `createSyncObjects()` called after `recreateSwapChain()` and `toggleHdr()`
- Command buffer index wrapped via `m_currentFrame % size`
- Baseline: 1000→6246 FPS, 100K→1940, 500K→593, 1M→336, 2M→179, 5M→97
- `benchmark.sh` for automated multi-count testing
- **Commit**: `047fdb8`

## Known Issues
- HDR output not visible on virtual display (Xvfb) — requires physical HDR monitor
- Windows build not yet tested
