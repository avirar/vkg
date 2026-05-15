#pragma once

#include "engine.h"
#include "compute.h"
#include "textures.h"

struct SunPushConstants {
    float centerX;
    float centerY;
    float scale;
    float alpha;
};

class Renderer {
public:
    Renderer(Engine& engine);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setCompute(Compute* compute) { m_compute = compute; }
    void setTextures(Textures* tex) { m_textures = tex; createPipelines(); }
    void drawFrame(float sinRot, float cosRot,
                   float singX, float singY, float singZ,
                   float aspectX, float aspectY);

private:
    void createCommandBuffers();
    void createPipelines();
    void createGraphicsPipeline();
    void createSunPipeline();
    void createSunVertexBuffer();

    Engine& m_engine;
    Compute* m_compute = nullptr;
    Textures* m_textures = nullptr;

    // Particle pipeline
    VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    // Sun pipeline
    VkPipelineLayout m_sunPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_sunPipeline = VK_NULL_HANDLE;
    VkBuffer m_sunVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_sunVertexMemory = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_currentFrame = 0;
};
