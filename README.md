# GL Gravitation — Vulkan Port (vkg)

Faithful recreation of the classic "GL Gravitation" OpenGL screensaver using **Vulkan compute + point-sprite rendering**.
Scales from 1K to **25 million particles at 120 FPS** with auto-scaling point size.

## Features

- **GPU compute physics** — gravity, damping, velocity, projection via compute shader
- **Point-sprite rendering** — one vertex per particle, fragment shader analytical glow (zero texture bandwidth)
- **Saturation blending** — `ONE_MINUS_DST_COLOR` prevents over-bright clipping, natural accumulation
- **Hypercolor** — 3 modes: `off` (static base color), `color` (velocity→hue shift), `brightness` (velocity boosts brightness)
- **Native HDR** — scRGB (16-bit float) or HDR10 PQ swapchain with BT.2020 metadata
- **Auto-scaling point size** — points shrink at higher counts (`1/sqrt(count/100K)`) to keep fragment load constant
- **7-layer instanced sun** — single draw call via `gl_InstanceIndex`
- **Unbounded particle count** — no compile-time hardcap; GPU buffers sized to actual count
- **Benchmark mode** — `--benchmark` with `IMMEDIATE` present mode for throughput measurement
- **Config-driven** — `vkg.ini` for all settings

## Building

### Dependencies (Arch)
```bash
sudo pacman -S vulkan-headers vulkan-icd-loader glfw glm shaderc cmake gcc
```

### Build
```bash
cd vkg
mkdir build && cd build
cmake ..
cmake --build .
./vkg
```

### Test without display
```bash
Xvfb :99 -screen 0 800x600x24 &
DISPLAY=:99 ./build/vkg --debug
```

## Configuration (`vkg.ini`)

Auto-loaded from the executable's directory. Default values shown:

| Key | Default | Description |
|-----|---------|-------------|
| `particles` | `1000000` | Starting particle count (no hardcap) |
| `fullscreen` | `true` | Start in fullscreen |
| `target_fps` | `0` | Target FPS for auto-adjuster (`0` = auto from display) |
| `point_scale` | `1.0` | Particle size multiplier (0.1–10.0) |
| `auto_point_scale` | `true` | Auto-shrink points at higher counts |
| `hypercolor_mode` | `brightness` | `off`, `color`, or `brightness` |
| `hyper_intensity` | `8.0` | Velocity→hue/brightness sensitivity |
| `hyper_lo_r/g/b` | `1.0 / 0.19 / 0.065` | Base particle color (RGB 0.0–1.0) |
| `hyper_hi_r/g/b` | `0.6 / 0.8 / 1.0` | High-velocity color for `color` mode only |
| `hdr` | `true` | Native HDR via scRGB or HDR10 PQ |
| `hdr_max_luminance` | `1000` | Peak display luminance in nits |

## CLI Flags

| Flag | Description |
|------|-------------|
| `--particles N` | Override particle count (min: 2) |
| `--fullscreen` | Force fullscreen at startup |
| `--point-scale F` | Override point size (0.1–10.0) |
| `--benchmark [S]` | Benchmark mode: no vsync, run S seconds (default 5), print avg FPS |
| `--debug` | Low-res 80×60 debug mode with ASCII pixel dump |
| `--help, -h` | Show help |

## Controls

| Key | Action |
|-----|--------|
| ESC | Exit |
| Space | Pause / Resume |
| F | Toggle fullscreen |
| H | Toggle HDR on/off |

## Performance

All figures with auto point scale enabled. Benchmark via `./benchmark.sh`.

| Particles | FPS |
|-----------|-----|
| 1,000 | 6,605 |
| 10,000 | 6,533 |
| 100,000 | 3,875 |
| 500,000 | 2,325 |
| 1,000,000 | 1,757 |
| 2,000,000 | 1,149 |
| 5,000,000 | 531 |
| 15,000,000 | 194 |
| **25,000,000** | **124** |

Cumulative 5.5× improvement from baseline (97 FPS at 5M → 531 FPS).

## Architecture

| Component | Choice |
|-----------|--------|
| Window | GLFW 3.4+ |
| Graphics API | Vulkan 1.2 |
| Build system | CMake |
| Language | C++17 |
| Shader compiler | glslc (shaderc) |

## Rendering Design

- **Compute shader** — 256-thread workgroups, double-buffered storage buffers (ping-pong)
- **Point primitives** — `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`, 40-byte `/Particle` vertex stride
- **Analytical glow** — fragment shader computes `dot(center,center)` quadratic falloff; zero texture bandwidth, no `sqrt`/`smoothstep`
- **Early discard** — fragments outside the point circle skip ROP (~50% savings)
- **Brightness-proportional point size** — dim particles cover fewer pixels
- **Saturation blend** — `ONE_MINUS_DST_COLOR` / `ONE` / `ADD` for color; `ONE_MINUS_DST_ALPHA` / `ONE` / `ADD` for alpha
- **Sun** — 7 concentric instanced quads projected from 3D singularity position

## Original Physics Constants (from `glg.c`)

| Constant | Value | Source |
|----------|-------|--------|
| Gravity | `0.01` | `glg.c:2522` |
| Damping | `0.982` | `glg.c:2524` |
| Position bounds | `[-1.0, 1.0]` | `glg.c:2537-2563` |
| Singularity vel range | `0.08` | `glg.c:2306-2326` |
| Singularity pos range | `0.2` | `glg.c:2332-2352` |
| Camera distance | `1.5` | `glg.c:2356, 2567` |
| Brightness falloff | `0.6666` | `glg.c:2571` |
| Rotation speed | `10.0 * dt` deg/s | `glg.c:2271` |
| Particle initial range | `[-0.15, 0.15]` | `glg.c:898-914` |
| Sun layers | 7 concentric quads | `glg.c:2377-2485` |

## References

| Resource | Link |
|----------|------|
| Original binary (Ghidra decompiled) | `glg-test/Ghidra C/glg.c` |
| Python reference port (physics verified) | `glg/particle_sim.py` |
| Missing features analysis | `glg/MISSING_FEATURES.md` |
| Tuning guide | `glg/TUNING_GUIDE.md` |
| Particle rendering technique (Acerola) | https://www.youtube.com/watch?v=1L-x_DH3Uvg |
| Vulkan specification / guide | https://github.com/KhronosGroup/Vulkan-Guide |
| Upstream repo | https://github.com/avirar/vkg |
