#include "engine.h"
#include "render.h"
#include "compute.h"
#include "simulation.h"
#include "textures.h"
#include "audio.h"
#include <iostream>
#include <chrono>
#include <cmath>
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

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GL Gravitation Vulkan", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    try {
        Engine engine(window);
        g_engine = &engine;

        Simulation sim(1000);
        Compute compute(engine);
        compute.init(sim.state().particleCount);

        Renderer renderer(engine);
        renderer.setCompute(&compute);

        Textures textures(engine);
        textures.createProceduralTextures();
        renderer.setTextures(&textures);

        Audio audio;
        // audio disabled by default; uncomment to enable:
        // audio.load("glg.wav");

        auto lastTime = std::chrono::high_resolution_clock::now();
        bool paused = false;
        bool fullscreen = false;
        std::unordered_map<int, bool> prevKeys;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            // Key controls (single-press)
            if (keyPressed(window, GLFW_KEY_SPACE, prevKeys)) {
                paused = !paused;
            }
            if (keyPressed(window, GLFW_KEY_R, prevKeys)) {
                sim.forceReinit();
            }
            if (keyPressed(window, GLFW_KEY_F, prevKeys)) {
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
            float angleRad = s.rotationAngle * 3.14159265f / 180.0f;
            bool doReinit = sim.justReinitialized();
            if (doReinit) {
                audio.play();
                sim.clearReinitFlag();
            }
            compute.update(paused ? 0.0f : dt,
                s.singularityX, s.singularityY, s.singularityZ,
                std::sin(angleRad), std::cos(angleRad),
                s.aspectRatioX, s.aspectRatioY,
                doReinit);

            if (!engine.beginFrame()) continue;
            renderer.drawFrame(std::sin(angleRad), std::cos(angleRad),
                              s.singularityX, s.singularityY, s.singularityZ,
                              s.aspectRatioX, s.aspectRatioY);
            engine.endFrame();
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
