# Progress

## Milestone 1 — Vulkan Window + Clear ✅
- GLFW window creation
- Vulkan instance, physical device, logical device
- Swapchain, render pass, framebuffers
- Sync objects (semaphores, fences)
- Frame loop: acquire → clear dark blue → present
- **Commit**: `6c50f58`

## Milestone 2 — Compute Pipeline ✅
- Descriptor set layout (storage buffer)
- Pipeline layout (push constants for simulation params)
- Particle compute shader (`particle.comp`) compiled to SPIR-V
- Compute pipeline creation
- Double-buffered particle storage (input/output ping-pong)
- Staging buffer for initial particle data (cube distribution [-0.15, 0.15])
- Descriptor pool and 2 sets (one per buffer)
- Per-frame dispatch: bind pipeline, push constants, dispatch N/256 workgroups
- Memory barrier: `COMPUTE_SHADER → VERTEX_INPUT` for vertex shader reads
- Compute::update() tracks simulation parameters
- **Commit**: `00961a5`

## Milestone 3 — Point-Sprite Rendering ✅
- Vertex shader reads screen_x/screen_y/brightness from particle buffer
- Fragment shader renders soft circles via `gl_PointCoord`
- Graphics pipeline: `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`, additive blending
- Vertex input: stride=sizeof(Particle), attributes at offsets
- Additive blend: `SRC_ALPHA` / `ONE` / `ADD` (matches `glg.c:878-879`)
- Draw: `vkCmdDraw(particleCount, 1, 0, 0)`
- **Commit**: `00961a5`

## Milestone 4 — Simulation State (CPU) ✅
- Rotation update: `10.0 * dt` deg/s (matches `glg.c:2271`)
- Singularity random walk velocity with [±0.08] clamping (matches `glg.c:2300-2326`)
- Singularity position update with [±0.2] boundary reflection (matches `glg.c:2332-2352`)
- Countdown timer: 2048 frames → reinit (matches `glg.c:2647`)
- Dynamic particle count: `sqrt(target/dt) * count` (matches `glg.c:2633-2646`)
- Aspect ratio handling (matches original WM_SIZE handler)
- Camera distance 1.5, offset 0.0 (matches `glg.c:2356, 2567`)
- **Commit**: (pending)

## Milestone 5 — Reinit in Compute Shader ✅
- Reinit flag in push constants triggers spherical particle distribution
- PCG hash for deterministic reinit per particle
- Spherical coordinates → Cartesian (matches `glg.c:2670-2690`)
- Single-frame reinit, then normal physics resumes
- **Commit**: (pending)

## Milestone 6 — Singularity Sun Rendering ✅
- Sun vertex shader: triangle strip quad + push constants (center, scale, alpha)
- Sun fragment shader: soft radial glow with blue color RGB(0.07, 0.30, 1.0)
- 7 concentric layers matching original (0.20, 0.08, 0.04, 0.02, 0.02, 0.01, 0.01)
- Sun position projected from 3D singularity via rotation + perspective
- Additive blending for glow accumulation
- **Commit**: (pending)

## Milestone 7 — Procedural Textures ⬜
- [ ] 64×64 RGBA sun glow texture
- [ ] 16×16 RGBA particle glow texture
- [ ] Texture sampler for fragment shaders

## Milestone 8 — Audio ⬜
- [ ] miniaudio integration
- [ ] Play glg.wav (MPEG layer 3) on reinit

## Milestone 9 — Polish ⬜
- [ ] Key controls (ESC, Space, R, G/g, D/d, +/-, A, F)
- [ ] Fullscreen toggle
- [ ] Debug UI overlay (FPS, particle count)
- [ ] Windows build support (MSVC/MinGW CMake toolchain)

## Known Issues
- Shader SPV files must be in CWD; eventually use executable-relative paths
- No validation layers installed on test system
- Need `Xvfb` for headless testing
- Particle count dynamic adjustment doesn't trigger buffer resize yet
