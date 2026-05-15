#pragma once

#include "engine.h"

class Renderer {
public:
    Renderer(Engine& engine);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void beginFrame();
    void endFrame();
    void drawFrame();

    VkCommandBuffer currentCmd() const { return m_commandBuffers[m_currentFrame]; }

private:
    void createCommandBuffers();

    Engine& m_engine;
    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_currentFrame = 0;
};
