#include "engine.h"
#include "config.h"
#include "render.h"
#include "compute.h"
#include "simulation.h"
#include "textures.h"
#include "audio.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <cstring>
#include <unordered_map>

static Engine* g_engine = nullptr;

static void framebufferResizeCallback(GLFWwindow* window, int w, int h) {
    if (g_engine) g_engine->setFramebufferResized();
}

static void keyCallback(GLFWwindow*, int, int, int, int) {}

static bool keyPressed(GLFWwindow* window, int key, std::unordered_map<int, bool>& prev) {
    bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    bool result = pressed && !prev[key];
    prev[key] = pressed;
    return result;
}

int main(int argc, char** argv) {
    Config cfg = Config::load();

    bool debugMode = false;
    bool startFullscreen = cfg.fullscreen;

    // CLI argument parsing
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            debugMode = true;
        } else if (std::strcmp(argv[i], "--particles") == 0 && i + 1 < argc) {
            cfg.particles = std::atoi(argv[++i]);
            if (cfg.particles < 2) cfg.particles = 2;
            if (cfg.particles > 32768) cfg.particles = 32768;
        } else if (std::strcmp(argv[i], "--fullscreen") == 0) {
            startFullscreen = true;
        } else if (std::strcmp(argv[i], "--point-scale") == 0 && i + 1 < argc) {
            cfg.pointScale = (float)std::atof(argv[++i]);
            if (cfg.pointScale < 0.1f) cfg.pointScale = 0.1f;
            if (cfg.pointScale > 10.0f) cfg.pointScale = 10.0f;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "vkg — Vulkan GL Gravitation screensaver\n"
                      << "Usage: vkg [options]\n"
                      << "  --debug          Low-res 80x60 debug mode, 1 frame\n"
                      << "  --particles N    Number of particles (2-32768, default: 1000)\n"
                      << "  --point-scale F  Particle size multiplier (0.1-10.0, default: 1.0)\n"
                      << "  --fullscreen     Start in fullscreen mode\n"
                      << "  --help, -h       Show this help\n"
                      << "\nConfig file: vkg.ini (auto-loaded from current directory)\n";
            return 0;
        }
    }

    int winW = debugMode ? 80 : 800;
    int winH = debugMode ? 60 : 600;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(winW, winH, "GL Gravitation Vulkan", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    // Apply fullscreen from config
    if (startFullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }

    try {
        Engine engine(window);
        g_engine = &engine;

        Simulation sim(debugMode ? 1 : (uint32_t)cfg.particles);
        if (cfg.targetFps > 0) sim.setTargetFps(cfg.targetFps);
        Compute compute(engine);
        compute.init(sim.state().particleCount);

        Renderer renderer(engine);
        renderer.setCompute(&compute);
        renderer.setDebug(debugMode);
        renderer.setPointScale(cfg.pointScale);

        Textures textures(engine);
        textures.createProceduralTextures();
        renderer.setTextures(&textures);

        Audio audio;
        if (!debugMode) {
            // audio.load("glg.wav");
        }

        auto lastTime = std::chrono::high_resolution_clock::now();
        bool paused = false;
        bool fullscreen = startFullscreen;
        std::unordered_map<int, bool> prevKeys;
        int debugFrameCount = 0;

        int fpsFrames = 0;
        auto fpsLastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            if (debugMode) {
                debugFrameCount++;
                if (debugFrameCount > 1) break;
            }

            // Key controls (single-press)
            if (!debugMode && keyPressed(window, GLFW_KEY_SPACE, prevKeys)) {
                paused = !paused;
            }
            if (!debugMode && keyPressed(window, GLFW_KEY_F, prevKeys)) {
                fullscreen = !fullscreen;
                GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
                const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                if (fullscreen) {
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                } else {
                    glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, 0);
                }
            }

            // Handle resize
            {
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                if (w > 0 && h > 0) sim.resize((uint32_t)w, (uint32_t)h);
            }

            // Delta time
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.1f) dt = 0.1f;

            // Update simulation (skip if paused)
            if (dt > 0.0f && !paused) {
                sim.update(dt);
            }

            // Feed simulation state to compute shader
            const auto& s = sim.state();
            float orbitRad = s.orbitAngle * 3.14159265f / 180.0f;
            float elevAngle = std::sin(s.wobblePhase * 0.7f) * 5.0f * 3.14159265f / 180.0f;
            elevAngle += std::sin(s.wobblePhase * 1.3f + 1.0f) * 2.0f * 3.14159265f / 180.0f;
            compute.update(paused ? 0.0f : dt,
                s.singularityX, s.singularityY, s.singularityZ,
                std::sin(orbitRad), std::cos(orbitRad),
                std::sin(elevAngle), std::cos(elevAngle),
                s.aspectRatioX, s.aspectRatioY);

            if (debugMode)
                compute.forceParticleCount(1);

            if (!engine.beginFrame()) continue;
            renderer.drawFrame(std::sin(orbitRad), std::cos(orbitRad),
                              std::sin(elevAngle), std::cos(elevAngle),
                              s.singularityX, s.singularityY, s.singularityZ,
                              s.aspectRatioX, s.aspectRatioY,
                              s.sunPulse);
            engine.endFrame();

            fpsFrames++;
            auto fpsNow = std::chrono::high_resolution_clock::now();
            float fpsElapsed = std::chrono::duration_cast<std::chrono::duration<float>>(fpsNow - fpsLastTime).count();
            if (fpsElapsed >= 1.0f) {
                int fps = (int)std::round(fpsFrames / fpsElapsed);
                glfwSetWindowTitle(window, ("GL Gravitation Vulkan - " + std::to_string(fps) + " FPS").c_str());
                fpsFrames = 0;
                fpsLastTime = fpsNow;
            }
        }

        engine.waitIdle();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
