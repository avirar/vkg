#pragma once

#include "engine.h"
#include <vector>
#include <algorithm>

struct ComputePushConstants {
    float dt;
    float gravity;
    float damping;
    float forceMult;
    uint32_t particleCount;
    uint32_t singCount;
    float comX;
    float comY;
    float comZ;
    float sinOrbit;
    float cosOrbit;
    float sinElev;
    float cosElev;
    float cameraDist;
    float cameraOffset;
    float aspectRatioX;
    float aspectRatioY;
    uint32_t seed;
    float debugMode;
    float dbgScrX;
    float dbgScrY;
    float dbgBright;
    float velocityIntensity;
    float distanceIntensity;
};
static_assert(sizeof(ComputePushConstants) == 96, "Must match GLSL layout");

struct SingData {
    float x, y, z, pad;
};
static_assert(sizeof(SingData) == 16, "Single vec4 in GLSL");

class Compute {
public:
    Compute(Engine& engine);
    ~Compute();

    void init(uint32_t particleCount);
    void recordInitialParticles(VkCommandBuffer cmd);
    void cleanupInitStaging();
    void update(float dt, uint32_t singCount, const SingData* singData,
                float comX, float comY, float comZ,
                float sinOrbit, float cosOrbit, float sinElev, float cosElev,
                float aspectRatioX, float aspectRatioY);
    void dispatch(VkCommandBuffer cmd);

    VkBuffer outputBuffer() const { return m_particleBuffers[m_outputIndex]; }
    uint32_t particleCount() const { return m_particleCount; }
    void forceParticleCount(uint32_t n) { m_particleCount = std::min(n, m_maxParticles); m_push.particleCount = m_particleCount; }
    void setVelocityIntensity(float vi) { m_push.velocityIntensity = vi; }
    void setDistanceIntensity(float di) { m_push.distanceIntensity = di; }
    void debugPlaceParticle(VkCommandBuffer cmd, float screenX, float screenY, float brightness);

private:
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createParticleBuffers();
    void createSingularityBuffer();
    void createDescriptorSets();

    Engine& m_engine;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSetAB = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSetBA = VK_NULL_HANDLE;

    std::vector<VkBuffer> m_particleBuffers;
    std::vector<VkDeviceMemory> m_particleBufferMemories;

    VkBuffer m_singBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_singMemory = VK_NULL_HANDLE;
    SingData m_singData[8]{};

    uint32_t m_particleCount = 0;
    uint32_t m_maxParticles = 0;
    uint32_t m_outputIndex = 0;
    uint32_t m_activeSet = 0;

    ComputePushConstants m_push{};
    VkPhysicalDeviceProperties m_deviceProps{};
    uint32_t m_seedCounter = 0;
    VkBuffer m_initStagingBuf = VK_NULL_HANDLE;
    VkDeviceMemory m_initStagingMem = VK_NULL_HANDLE;
};
