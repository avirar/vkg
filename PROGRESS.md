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
- **Commit**: (pending)

## Known Issues
- Windows build not yet tested (CMake toolchain should work)
- Audio currently disabled (commented out load call)
- HDR not yet implemented
- No indirect draw / GPU-driven particle count yet
