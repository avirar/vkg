#include "engine.h"
#include "render.h"
#include "compute.h"
#include "simulation.h"
#include <iostream>
#include <chrono>
#include <cmath>

static Engine* g_engine = nullptr;

static void framebufferResizeCallback(GLFWwindow* window, int w, int h) {
    if (g_engine) {
        g_engine->setFramebufferResized();
    }
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

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

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

            // Update simulation
            if (dt > 0.0f) {
                sim.update(dt);
            }

            // Feed simulation state to compute shader
            const auto& s = sim.state();
            float angleRad = s.rotationAngle * 3.14159265f / 180.0f;
            bool doReinit = sim.justReinitialized();
            compute.update(dt,
                s.singularityX, s.singularityY, s.singularityZ,
                std::sin(angleRad), std::cos(angleRad),
                s.aspectRatioX, s.aspectRatioY,
                doReinit);
            if (doReinit) sim.clearReinitFlag();

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
