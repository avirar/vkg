# GL Gravitation — Vulkan Port (vkg)

Faithful recreation of the classic "GL Gravitation" OpenGL screensaver using **Vulkan compute + point-sprite rendering**.

## References

| Resource | Link |
|---|---|
| Original binary (ghidra decompiled) | `glg-test/Ghidra C/glg.c` |
| Python reference port (physics verified) | `glg/particle_sim.py` (in `glg.7z`) |
| Missing features analysis | `glg/MISSING_FEATURES.md` (in `glg.7z`) |
| Tuning guide | `glg/TUNING_GUIDE.md` (in `glg.7z`) |
| Particle rendering technique (Acerola) | https://www.youtube.com/watch?v=1L-x_DH3Uvg |
| Vulkan specification / guide | https://github.com/KhronosGroup/Vulkan-Guide |
| Upstream repo | https://github.com/avirar/vkg |

## Original Physics Constants (from `glg.c`)

| Constant | Value | Source |
|---|---|---|
| Gravity | `0.01` | `glg.c:2522` |
| Damping | `0.982` | `glg.c:2524` |
| Position bounds | `[-1.0, 1.0]` | `glg.c:2537-2563` |
| Singularity vel range | `0.08` | `glg.c:2306-2326` |
| Singularity pos range | `0.2` | `glg.c:2332-2352` |
| Camera distance | `1.5` | `glg.c:2356, 2567` |
| Brightness falloff | `0.6666` | `glg.c:2571` |
| Reinit countdown | 2048 frames | `glg.c:1251` |
| Max particles | 32768 | `glg.c:2645` |
| Rotation speed | `10.0 * dt` deg/s | `glg.c:2271` |
| Particle initial cube range | `[-0.15, 0.15]` | `glg.c:898-914` |
| Sphere init radius | `0.004472136` | `glg.c:2675-2682` |
| Sun layers | 7 concentric quads | `glg.c:2377-2485` |

## Original Blending (from `glg.c:878-879`)

```c
glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blending
```
Vulkan equivalent: `VK_BLEND_FACTOR_SRC_ALPHA` / `VK_BLEND_FACTOR_ONE` / `VK_BLEND_OP_ADD`

## Architecture

| Component | Choice |
|---|---|
| Window | GLFW 3.4+ |
| Graphics API | Vulkan 1.2 |
| Build system | CMake |
| Language | C++17 |
| Math library | GLM (header-only) |
| Memory allocation | VMA (single header) |
| Audio (planned) | miniaudio (single header) |
| Shader compiler | glslc (shaderc) |

## Rendering Design

- **Physics**: GPU compute shader — one invocation per particle, gravitational N-body
- **Rendering**: Point primitives (`VK_PRIMITIVE_TOPOLOGY_POINT_LIST`) — one vertex per particle
- **Point expansion**: Fragment shader renders soft circles via `gl_PointCoord`
- **Blending**: Additive (`SRC_ALPHA`, `ONE`) — overlapping particles accumulate brightness
- **Particle buffer**: Double-buffered storage buffer (compute output → vertex input)

## Building

### Dependencies (Arch)
```bash
sudo pacman -S vulkan-headers vulkan-icd-loader glfw glm shaderc spirv-tools cmake gcc
```

### Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
./vkg
```

### Test without display
```bash
Xvfb :99 -screen 0 800x600x24 &
DISPLAY=:99 ./build/vkg
```

## Controls

| Key | Action |
|---|---|
| ESC | Exit |
| Space | Pause / Resume |
| R | Force reinitialization |
| F | Toggle fullscreen |

## Notes

- All 9 milestones implemented — compute physics, point-sprite rendering, textured sun, audio
- Uses Vulkan 1.2 with compute shader for GPU particle physics
- Scales to 32K particles (configurable via `MAX_PARTICLES`)
- Particle count dynamically adjusts based on framerate
- Additive blending matches the original's accumulation behavior
