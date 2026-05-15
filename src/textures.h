#pragma once

#include "engine.h"

class Textures {
public:
    Textures(Engine& engine);
    ~Textures();

    void createProceduralTextures();

    VkImageView sunTextureView() const { return m_sunTextureView; }
    VkImageView particleTextureView() const { return m_particleTextureView; }
    VkSampler sampler() const { return m_sampler; }

private:
    Engine& m_engine;
    VkImage m_sunTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_sunTextureMemory = VK_NULL_HANDLE;
    VkImageView m_sunTextureView = VK_NULL_HANDLE;
    VkImage m_particleTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_particleTextureMemory = VK_NULL_HANDLE;
    VkImageView m_particleTextureView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
};
