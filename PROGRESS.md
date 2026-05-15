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
- Rotation: 10.0°/s * dt, singularity wobble, boundary reflection
- Countdown timer: 2048 frames → reinit
- Dynamic particle count: `sqrt(target/dt) * count`
- Aspect ratio, camera distance 1.5, offset 0.0
- **Commit**: `7a39c02`

## Milestone 5 — Reinit in Compute Shader ✅
- Reinit flag triggers spherical particle distribution via PCG hash
- Single-frame reinit, then normal physics resumes
- **Commit**: `7a39c02`

## Milestone 6 — Singularity Sun Rendering ✅
- 7 concentric triangle-strip layers (matching original sizes)
- Sun position projected from 3D singularity
- Blue glow color: RGB(0.07, 0.30, 1.0)
- **Commit**: `0141e9f`

## Milestone 7 — Procedural Textures ✅
- 64×64 radial gradient for sun glow
- 16×16 radial gradient for particle glow
- Texture sampler + dual descriptor sets
- **Commit**: `9bdfe6c`

## Milestone 8 — Audio ✅
- miniaudio integration — plays glg.wav on reinit
- Graceful fallback if file missing
- **Commit**: `6b7ee0b`

## Milestone 9 — Polish ✅
- Key controls: ESC (quit), Space (pause), R (reinit), F (fullscreen)
- Pause freezes simulation (dt=0 in compute shader)
- Fullscreen toggle via GLFW monitor switch
- **Commit**: (pending)

## Known Issues
- Shader SPVs loaded relative to CWD
- No validation layers on test system
- Need Xvfb for headless testing
- Particle count dynamic adjustment doesn't trigger buffer resize yet
- Windows build not yet tested (CMake toolchain should work)
