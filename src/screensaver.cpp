#ifdef _WIN32

#ifndef UNICODE
#define UNICODE
#endif

#include "engine.h"
#include "config.h"
#include "render.h"
#include "compute.h"
#include "simulation.h"
#include "textures.h"
#include <windows.h>
#include <string>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const wchar_t* WINDOW_CLASS = L"vkgScreenSaverClass";
static bool g_running = true;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
        case WM_CLOSE:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEMOVE:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_SYSCOMMAND:
            if (wParam == SC_SCREENSAVE || wParam == SC_CLOSE) return 0;
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static HWND createFullscreenWindow(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = nullptr;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    return CreateWindowExW(WS_EX_TOPMOST | WS_EX_APPWINDOW,
        WINDOW_CLASS, L"vkg", WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH, nullptr, nullptr, hInstance, nullptr);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
    std::string args(lpCmdLine);

    // Trim leading/trailing whitespace
    while (!args.empty() && (args.front() == ' ' || args.front() == '\t')) args.erase(0, 1);
    while (!args.empty() && (args.back() == ' ' || args.back() == '\t')) args.pop_back();

    // Tokenize: get first token (the mode flag) and the rest
    std::string token, rest;
    size_t sp = args.find_first_of(" \t");
    if (sp != std::string::npos) {
        token = args.substr(0, sp);
        rest = args.substr(sp);
        while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) rest.erase(0, 1);
    } else {
        token = args;
    }

    // Normalize: strip leading / or - then lowercase
    std::string mode;
    if (!token.empty() && (token[0] == '/' || token[0] == '-'))
        mode = token.substr(1);
    else
        mode = token;
    for (auto& c : mode) c = static_cast<char>(std::tolower(c));

    bool isConfig = false;
    bool isPreview = false;
    HWND parentHwnd = nullptr;

    if (mode == "s") {
        // Run screensaver fullscreen
    } else if (mode == "p") {
        // Preview: render as child of supplied HWND
        isPreview = true;
        parentHwnd = reinterpret_cast<HWND>(static_cast<UINT_PTR>(
            rest.empty() ? 0 : std::stoull(rest)));
    } else if (mode == "c" || mode.empty()) {
        // Configure (also default when no args per MS spec)
        isConfig = true;
    } else if (mode == "a") {
        // Change password (legacy, ignored)
        return 0;
    }

    if (isConfig) {
        // Load config to show current values
        Config cfg = Config::load();
        char msg[512];
        snprintf(msg, sizeof(msg),
            "vkg — Vulkan GL Gravitation\n\n"
            "Configure via vkg.ini in the screensaver directory.\n\n"
            "Current settings:\n"
            "  particles: %d\n"
            "  hypercolor_velocity_mode: %s\n"
            "  hypercolor_distance_mode: %s\n"
            "  velocity intensity: %.1f\n"
            "  distance intensity: %.1f\n"
            "  blend_alpha_scale: %.2f\n"
            "  color_cap: %.2f\n"
            "  singularity_count: %d\n"
            "  target_fps: %d (0=auto)\n"
            "  HDR: %s",
            cfg.particles,
            cfg.hypercolorVelocityMode.c_str(),
            cfg.hypercolorDistanceMode.c_str(),
            cfg.hyperVelocityIntensity,
            cfg.hyperDistanceIntensity,
            cfg.blendAlphaScale,
            cfg.colorCap,
            cfg.singularityCount,
            cfg.targetFps,
            cfg.hdr ? "on" : "off");
        MessageBoxA(nullptr, msg, "vkg Settings", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    HWND hwnd;
    RECT windowRect;

    if (isPreview) {
        // Per MS docs: create child window inside parent's client area
        GetClientRect(parentHwnd, &windowRect);
        int pw = windowRect.right - windowRect.left;
        int ph = windowRect.bottom - windowRect.top;
        if (pw <= 0) pw = 160;
        if (ph <= 0) ph = 120;

        // Register class for child preview window
        WNDCLASSEXW pwc = {};
        pwc.cbSize = sizeof(pwc);
        pwc.style = CS_HREDRAW | CS_VREDRAW;
        pwc.lpfnWndProc = WndProc;
        pwc.hInstance = hInstance;
        pwc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        pwc.lpszClassName = L"vkgPreviewClass";
        RegisterClassExW(&pwc);

        hwnd = CreateWindowExW(0, L"vkgPreviewClass", L"vkgPreview",
            WS_CHILD | WS_VISIBLE, 0, 0, pw, ph,
            parentHwnd, nullptr, hInstance, nullptr);
        if (!hwnd) return 1;

        windowRect.left = 0;
        windowRect.top = 0;
        windowRect.right = pw;
        windowRect.bottom = ph;
    } else {
        hwnd = createFullscreenWindow(hInstance, nCmdShow);
        if (!hwnd) return 1;
        windowRect.left = 0;
        windowRect.top = 0;
        windowRect.right = GetSystemMetrics(SM_CXSCREEN);
        windowRect.bottom = GetSystemMetrics(SM_CYSCREEN);
        ShowCursor(FALSE);
    }

    try {
        Config cfg = Config::load();

        Engine engine(hwnd, hInstance, cfg.hdr, false);
        if (engine.hdrEnabled()) {
            engine.setHdrMaxLuminance(cfg.hdrMaxLuminance);
        }

        Simulation sim(static_cast<uint32_t>(cfg.particles), static_cast<uint32_t>(cfg.singularityCount));
        if (cfg.targetFps > 0) sim.setTargetFps(cfg.targetFps);
        else sim.setTargetFps(static_cast<int>(engine.displayRefreshRate()));

        Compute compute(engine);
        compute.init(sim.state().particleCount);

        Renderer renderer(engine);
        renderer.setCompute(&compute);
        renderer.setPointScale(cfg.pointScale);
        renderer.setParticleColors(cfg);
        compute.setVelocityIntensity(cfg.hyperVelocityIntensity);
        compute.setDistanceIntensity(cfg.hyperDistanceIntensity);
        renderer.setOsd(cfg.osdEnabled);

        Textures textures(engine);
        textures.createProceduralTextures();
        renderer.setTextures(&textures);

        // Batch init transfers
        {
            VkCommandBuffer cmd = engine.beginSingleTimeCommands();
            compute.recordInitialParticles(cmd);
            renderer.recordSunGeometryInit(cmd);
            engine.endSingleTimeCommands(cmd);
            compute.cleanupInitStaging();
            renderer.cleanupSunInitStaging();
        }

        auto lastTime = std::chrono::high_resolution_clock::now();
        bool paused = false;
        float currentFps = 0.0f;
        int fpsFrames = 0;
        auto fpsLastTime = lastTime;

        while (g_running) {
            // Process Windows messages
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    g_running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (!g_running) break;

            // Delta time
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.1f) dt = 0.1f;

            // Update simulation
            if (dt > 0.0f && !paused) {
                sim.update(dt);
            }

            // Sync particle count
            compute.forceParticleCount(sim.state().particleCount);

            // Auto-scale point size
            if (cfg.autoPointScale) {
                static float lastAutoParticles = 0;
                static float lastAutoScale = 1.0f;
                float pc = static_cast<float>(compute.particleCount());
                if (std::abs(pc - lastAutoParticles) / (lastAutoParticles + 1.0f) > 0.1f) {
                    lastAutoParticles = pc;
                    lastAutoScale = 1.0f / std::sqrt(std::max(1.0f, pc / 100000.0f));
                }
                renderer.setPointScale(cfg.pointScale * lastAutoScale);
            }

            // Feed simulation state to compute
            const auto& s = sim.state();
            SingData singData[8]{};
            for (uint32_t i = 0; i < s.singularityCount && i < 8; i++) {
                singData[i].x = s.singularities[i].x;
                singData[i].y = s.singularities[i].y;
                singData[i].z = s.singularities[i].z;
                singData[i].pad = 0.0f;
            }
            float orbitRad = s.orbitAngle * 3.14159265f / 180.0f;
            float elevAngle = std::sin(s.wobblePhase * 0.7f) * 5.0f * 3.14159265f / 180.0f;
            elevAngle += std::sin(s.wobblePhase * 1.3f + 1.0f) * 2.0f * 3.14159265f / 180.0f;
            compute.update(paused ? 0.0f : dt,
                s.singularityCount, singData,
                s.comX, s.comY, s.comZ,
                std::sin(orbitRad), std::cos(orbitRad),
                std::sin(elevAngle), std::cos(elevAngle),
                s.aspectRatioX, s.aspectRatioY);

            if (!engine.beginFrame()) continue;
            renderer.setOsdStats(compute.particleCount(), currentFps);
            renderer.setOsdTargetFps(sim.targetFps());
            renderer.drawFrame(s, s.aspectRatioX, s.aspectRatioY);
            engine.endFrame();

            fpsFrames++;
            auto fpsNow = std::chrono::high_resolution_clock::now();
            float fpsElapsed = std::chrono::duration_cast<std::chrono::duration<float>>(fpsNow - fpsLastTime).count();
            if (fpsElapsed >= 1.0f) {
                currentFps = fpsFrames / fpsElapsed;
                fpsFrames = 0;
                fpsLastTime = fpsNow;
            }
        }

        engine.waitIdle();
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "vkg Error", MB_OK | MB_ICONERROR);
        if (isPreview) {
            DestroyWindow(hwnd);
        } else {
            ShowCursor(TRUE);
            DestroyWindow(hwnd);
        }
        return 1;
    }

    if (isPreview) {
        DestroyWindow(hwnd);
    } else {
        ShowCursor(TRUE);
        DestroyWindow(hwnd);
    }
    return 0;
}

#endif // _WIN32
