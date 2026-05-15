#pragma once

#include "engine.h"
#include "compute.h"

class Renderer {
public:
    Renderer(Engine& engine);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setCompute(Compute* compute) { m_compute = compute; }
    void drawFrame();

private:
    void createCommandBuffers();
    void createGraphicsPipeline();

    Engine& m_engine;
    Compute* m_compute = nullptr;

    VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_currentFrame = 0;
};
