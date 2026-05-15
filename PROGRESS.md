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
- **Commit**: (pending)

## Milestone 3 — Point-Sprite Rendering 🚧
- [x] Rewrite quad.vert for point primitive input (screen_x, screen_y, brightness)
- [x] Rewrite quad.frag for soft circle rendering via gl_PointCoord
- [ ] Create graphics pipeline with blend state (additive)
- [ ] Vertex input binding: stride=sizeof(Particle), attributes at offsets
- [ ] Bind particle buffer as vertex buffer
- [ ] Draw call: vkCmdDraw(particleCount, 1, 0, 0)
- [ ] Integrate compute dispatch + draw in render loop

## Milestone 4 — Singularity (Sun) ⬜
- [ ] Singularity CPU update (random walk velocity, boundary reflection)
- [ ] Sun vertex/fragment shaders (7 concentric layers)
- [ ] Sun pipeline + draw call

## Milestone 5 — Full Physics Integration ⬜
- [ ] Rotation update (10.0 deg/s * dt)
- [ ] Reinit countdown (2048 frames → sphere redistribution)
- [ ] Dynamic particle count (sqrt(target/actual) * count)
- [ ] Physics frozen during countdown
- [ ] Aspect ratio handling

## Milestone 6 — Procedural Textures ⬜
- [ ] 64×64 RGBA sun glow texture
- [ ] 16×16 RGBA particle glow texture
- [ ] Texture sampler for fragment shaders

## Milestone 7 — Audio ⬜
- [ ] miniaudio integration
- [ ] Play glg.wav (MPEG layer 3) on reinit

## Milestone 8 — Polish ⬜
- [ ] Key controls (ESC, Space, R, G/g, D/d, +/-, A, F)
- [ ] Fullscreen toggle
- [ ] Debug UI overlay (FPS, particle count)
- [ ] Windows build support (MSVC/MinGW CMake toolchain)

## Known Issues
- Shader SPV files must be in CWD; eventually use executable-relative paths
- No validation layers installed on test system
- Need `Xvfb` for headless testing
