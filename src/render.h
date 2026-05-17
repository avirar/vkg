#pragma once

#include "engine.h"
#include "compute.h"
#include "textures.h"
#include "config.h"
#include "simulation.h"

struct SunPushConstants {
    float centerX;
    float centerY;
    float aspectX;
    float sunPulse;
    float layerScales[7];
    float layerAlphas[7];
};

struct ParticlePushConstants {
    float viewportHeight;
    float aspectY;
    float pointSizeMult;
    uint32_t velMode;  // 0=off, 1=color, 2=brightness
    uint32_t distMode; // 0=off, 1=color, 2=brightness
    float velLoR, velLoG, velLoB;
    float velHiR, velHiG, velHiB;
    float distLoR, distLoG, distLoB;
    float distHiR, distHiG, distHiB;
    float blendAlphaScale;
    float colorCap;
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
    void setPointScale(float s) { m_pointScale = s; }
    void setParticleColors(const Config& cfg);
    void setOsd(bool osd) { m_osd = osd; }
    void setOsdStats(uint32_t count, float fps) { m_osdParticles = count; m_osdFps = fps; }
    void setOsdTargetFps(float fps) { m_osdTargetFps = fps; }
    void drawFrame(const SimState& state, float aspectX, float aspectY);
    void recordSunGeometryInit(VkCommandBuffer cmd);
    void cleanupSunInitStaging();

private:
    void createCommandBuffers();
    void createPipelines();
    void createGraphicsPipeline();
    void createSunPipeline();
    void createSunVertexBuffer();
    void createSunIndexBuffer();
    void createOsdPipeline();
    void drawOsd(VkCommandBuffer cmd, uint32_t particleCount, float fps);

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

    // OSD pipeline
    VkPipelineLayout m_osdPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_osdPipeline = VK_NULL_HANDLE;
    VkBuffer m_osdVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_osdVertexMemory = VK_NULL_HANDLE;
    VkBuffer m_osdIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_osdIndexMemory = VK_NULL_HANDLE;
    bool m_osd = false;
    uint32_t m_osdParticles = 0;
    float m_osdFps = 0.0f;
    float m_osdTargetFps = 0.0f;

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkBuffer m_ssBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_ssMemory = VK_NULL_HANDLE;
    bool m_ssCaptured = false;
    VkBuffer m_sunInitStagingVB = VK_NULL_HANDLE;
    VkDeviceMemory m_sunInitStagingVBMem = VK_NULL_HANDLE;
    VkBuffer m_sunInitStagingIB = VK_NULL_HANDLE;
    VkDeviceMemory m_sunInitStagingIBMem = VK_NULL_HANDLE;
    bool m_debugMode = false;
    float m_pointScale = 1.0f;
    ParticlePushConstants m_ppc{};
    void initScreenshotBuffer();
    void saveScreenshot(VkFence fence);
    void debugDump(VkFence fence);
};
