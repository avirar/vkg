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
    bool isPreview = false;
    HWND parentHwnd = nullptr;

    // Parse command-line arguments
    if (!args.empty()) {
        // Windows passes arguments with leading '/'
        if (args[0] == '/' || args[0] == '-') {
            char mode = static_cast<char>(std::tolower(args[1]));
            if (mode == 's') {
                // Run screensaver fullscreen
            } else if (mode == 'p') {
                // Preview mode: render into parent window
                isPreview = true;
                std::string hwndStr = args.substr(3);
                parentHwnd = reinterpret_cast<HWND>(static_cast<UINT_PTR>(std::stoull(hwndStr)));
            } else if (mode == 'c') {
                // Configure
                MessageBoxA(nullptr, "vkg — Vulkan GL Gravitation\n\nConfigure via vkg.ini in the same directory.",
                           "vkg Settings", MB_OK | MB_ICONINFORMATION);
                return 0;
            } else if (mode == 'a') {
                // Change password (legacy, ignored)
                return 0;
            }
        }
    }

    HWND hwnd;
    RECT windowRect;

    if (isPreview) {
        hwnd = parentHwnd;
        GetClientRect(hwnd, &windowRect);
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

        Compute compute(engine);
        compute.init(sim.state().particleCount);

        Renderer renderer(engine);
        renderer.setCompute(&compute);
        renderer.setPointScale(cfg.pointScale);
        renderer.setParticleColors(cfg);
        compute.setVelocityIntensity(cfg.hyperVelocityIntensity);
        compute.setDistanceIntensity(cfg.hyperDistanceIntensity);
        renderer.setOsd(false);

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
        if (!isPreview) {
            ShowCursor(TRUE);
            DestroyWindow(hwnd);
        }
        return 1;
    }

    if (!isPreview) {
        ShowCursor(TRUE);
        DestroyWindow(hwnd);
    }
    return 0;
}

#endif // _WIN32
