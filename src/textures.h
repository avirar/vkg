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
    VkDescriptorSet sunDescriptorSet() const { return m_sunDescriptorSet; }
    VkDescriptorSet particleDescriptorSet() const { return m_particleDescriptorSet; }
    VkDescriptorSetLayout descriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    void createImage(uint32_t w, uint32_t h, VkImage& image, VkDeviceMemory& memory);
    void createImageView(VkImage image, VkFormat format, VkImageView& view);
    void uploadTexture(VkImage image, uint32_t w, uint32_t h,
                       const uint8_t* pixels);
    void createSampler();
    void createDescriptorSet();

    Engine& m_engine;

    VkImage m_sunTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_sunTextureMemory = VK_NULL_HANDLE;
    VkImageView m_sunTextureView = VK_NULL_HANDLE;

    VkImage m_particleTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_particleTextureMemory = VK_NULL_HANDLE;
    VkImageView m_particleTextureView = VK_NULL_HANDLE;

    VkSampler m_sampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_sunDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet m_particleDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};
