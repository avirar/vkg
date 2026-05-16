#pragma once

#include "engine.h"
#include <vector>

struct ComputePushConstants {
    float dt;
    float gravity;
    float damping;
    uint32_t particleCount;
    float singularityX;
    float singularityY;
    float singularityZ;
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
};

class Compute {
public:
    Compute(Engine& engine);
    ~Compute();

    void init(uint32_t particleCount);
    void update(float dt, float singularityX, float singularityY, float singularityZ,
                float sinOrbit, float cosOrbit, float sinElev, float cosElev,
                float aspectRatioX, float aspectRatioY);
    void dispatch(VkCommandBuffer cmd);

    VkBuffer outputBuffer() const { return m_particleBuffers[m_outputIndex]; }
    uint32_t particleCount() const { return m_particleCount; }
    void forceParticleCount(uint32_t n) { m_particleCount = n; m_push.particleCount = n; }
    void debugPlaceParticle(VkCommandBuffer cmd, float screenX, float screenY, float brightness);

private:
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createParticleBuffers();
    void createDescriptorSets();
    void initializeParticles();

    Engine& m_engine;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSetAB = VK_NULL_HANDLE; // binding0=A, binding1=B
    VkDescriptorSet m_descriptorSetBA = VK_NULL_HANDLE; // binding0=B, binding1=A

    std::vector<VkBuffer> m_particleBuffers;
    std::vector<VkDeviceMemory> m_particleBufferMemories;

    uint32_t m_particleCount = 0;
    uint32_t m_outputIndex = 0; // which buffer is the current output (0 or 1)
    uint32_t m_activeSet = 0;   // 0 = use AB set, 1 = use BA set

    ComputePushConstants m_push{};
    VkPhysicalDeviceProperties m_deviceProps{};
};
