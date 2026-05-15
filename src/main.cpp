#include "engine.h"
#include "render.h"
#include "compute.h"
#include <iostream>
#include <chrono>
#include <cmath>

static Engine* g_engine = nullptr;

static void framebufferResizeCallback(GLFWwindow*, int, int) {
    if (g_engine) g_engine->setFramebufferResized();
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

        Compute compute(engine);
        compute.init(1000); // Start with 1000 particles

        Renderer renderer(engine);
        renderer.setCompute(&compute);

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.1f) dt = 0.1f; // Cap

            float angle = 0.0f; // placeholder rotation
            compute.update(dt, 0, 0, 0, std::sin(angle), std::cos(angle), 1.0f, 1.0f);

            if (!engine.beginFrame()) continue;

            renderer.drawFrame();
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
