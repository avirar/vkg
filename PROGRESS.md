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
- **Commit**: `088c807`

## Milestone 21 — Auto-Scaling Point Size ✅
- `auto_point_scale = true` (config/ini): points shrink at higher particle counts
- Formula: `scale = base / sqrt(max(1, count / 100000))` keeps fragment work constant
- 100K→1x, 500K→0.45x, 1M→0.32x, 2M→0.22x, 5M→0.14x auto-adaptive
- Perf: 500K 593→1253 (2.1x), 1M 336→889 (2.6x), 2M 179→623 (3.5x), 5M 97→371 (3.8x)
- `point_scale` still applies as baseline; auto scaling can be disabled
- Fragment/ROP confirmed as bottleneck (half point size ≈ 2x FPS)
- **Commit**: `aad7e2c`

## Milestone 22 — Tier 1 Fragment Shader + Blend Optimizations ✅
- **Blend saturation**: `ONE_MINUS_DST_COLOR` src factor → natural soft saturation as pixel fills up
- **Analytical glow**: replaced `texture(sampler, coord).r` with `dot(center,center)*8.16` quadratic — zero bandwidth
- **No sqrt in fragment**: `length()` replaced by `dot()`, `smoothstep` replaced by `clamp(1-d², 0,1)²`
- **Early discard**: `if (d >= 1.0) discard` saves ROP for ~50% of fragments outside the point
- **Brightness point size**: `gl_PointSize *= inBrightness` → dim particles cover fewer pixels
- Perf: 100K 1955→3753 (+92%), 1M 889→1734 (+95%), 5M 371→535 (+44%)
- **Commit**: `e11b454`

## Milestone 23 — Tier 2 CPU Optimizations ✅
- **P6**: Replaced `std::chrono::steady_clock::now()` with frame counter for compute seed — saves syscall/frame
- **P9**: Precomputed `forceMult = gravity * dt * (1 + damping)` — compute shader does 3 fewer muls/particle
- **P8**: Cached `autoPointScale` — `std::sqrt` only recomputes when count changes >10%
- (P5 compute sqrt deferred — compute is not the bottleneck; P7 brightness point size done in Tier 1)
- **Commit**: `0250415`

## Milestone 24 — Tier 3 Misc Optimizations ✅
- **P13**: Cleaned particle buffer usage flags (removed `VERTEX_BUFFER_BIT`, `TRANSFER_SRC_BIT` from physics-only buffers)
- (P11 double fence was false flag — already correct; P10 sun instancing deferred; P12 batch init deferred)
- **Commit**: `40c8050`
- **Note**: P13 was incorrect — particle buffers ARE used as vertex buffers (bindVertexBuffers in drawFrame). VERTEX_BUFFER_BIT restored in M27.

## Milestone 25 — Sun Instancing (P10) ✅
- Merged 7 sun draw calls into 1 instanced draw via `gl_InstanceIndex`
- SunPushConstants (72 bytes): `centerX, centerY, aspectX, sunPulse, layerScales[7], layerAlphas[7]`
- Vertex shader computes `scaleX = layerScales[i] * aspectX`, `alpha = layerAlphas[i] * sunPulse` per instance
- `vkCmdDrawIndexed(cmd, 6, 7, 0, 0, 0)` replaces 7 separate push+draw sequences
- Layer sizes: 0.20, 0.08, 0.04, 0.02, 0.02, 0.01, 0.01 (matching original)
- **Commit**: (this commit)

## Milestone 26 — Batch Init Transfers (P12) ✅
- Merged 3 separate `beginSingleTimeCommands/endSingleTimeCommands` init submissions into 1
- `Compute::recordInitialParticles(cmd)` — records particle buffer copies to shared cmd
- `Renderer::recordSunGeometryInit(cmd)` — records sun VB + IB copies to shared cmd  
- `Compute::cleanupInitStaging()` / `Renderer::cleanupSunInitStaging()` — deferred staging cleanup
- `createSunVertexBuffer/IndexBuffer` simplified to buffer-only allocation (no staging in constructor)
- Init pipe: 3 submits (sun VB + sun IB + particles) → 1 submit
- **Commit**: (this commit)

## Milestone 27 — Compute Sqrt Removal (P5) + VERTEX_BUFFER_BIT Fix ✅
- Removed `sqrt()` from compute shader — replaced with squared velocity magnitude: `velMagSq * hyperIntensity * 100.0`
- Compensating 100x factor for quadratic falloff at typical velocities (~0.01)
- Restored `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` to particle buffers (M24 P13 incorrectly removed it)
- Particle buffers need all 3 flags: STORAGE (compute r/w) | TRANSFER_DST (init copy) | VERTEX (bindVertexBuffers)
- **Commit**: (this commit)

## Performance (current)
- 100K: 3875 FPS | 500K: 2325 FPS | 1M: 1757 FPS | 2M: 1149 FPS | 5M: 531 FPS
- From baseline (97→531 at 5M): 5.5x cumulative improvement

## Known Issues
- HDR output not visible on virtual display (Xvfb) — requires physical HDR monitor
- Windows build not yet tested
