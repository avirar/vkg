#pragma once

#include "engine.h"
#include "compute.h"
#include "textures.h"

struct SunPushConstants {
    float centerX;
    float centerY;
    float scaleX;
    float scaleY;
    float alpha;
    float _pad;
};

struct ParticlePushConstants {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    float _pad;
};

class Renderer {
public:
    Renderer(Engine& engine);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setCompute(Compute* compute) { m_compute = compute; }
    void setTextures(Textures* tex) { m_textures = tex; createPipelines(); }
    void setDebug(bool d) { m_debugMode = d; }
    void drawFrame(float sinOrbit, float cosOrbit,
                   float sinElev, float cosElev,
                   float singX, float singY, float singZ,
                   float aspectX, float aspectY);

private:
    void createCommandBuffers();
    void createPipelines();
    void createGraphicsPipeline();
    void createSunPipeline();
    void createSunVertexBuffer();
    void createSunIndexBuffer();

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
    VkBuffer m_sunIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_sunIndexMemory = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkBuffer m_ssBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_ssMemory = VK_NULL_HANDLE;
    bool m_ssCaptured = false;
    bool m_debugMode = false;
    void initScreenshotBuffer();
    void saveScreenshot(VkFence fence);
    void debugDump(VkFence fence);
};
