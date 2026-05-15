#include "textures.h"
#include <cstring>
#include <cmath>
#include <vector>

Textures::Textures(Engine& engine) : m_engine(engine) {}

Textures::~Textures() {
    vkDeviceWaitIdle(m_engine.device());
    if (m_sampler) vkDestroySampler(m_engine.device(), m_sampler, nullptr);
    if (m_descriptorSetLayout) vkDestroyDescriptorSetLayout(m_engine.device(), m_descriptorSetLayout, nullptr);
    if (m_descriptorPool) vkDestroyDescriptorPool(m_engine.device(), m_descriptorPool, nullptr);
    if (m_sunTextureView) vkDestroyImageView(m_engine.device(), m_sunTextureView, nullptr);
    if (m_sunTexture) vkDestroyImage(m_engine.device(), m_sunTexture, nullptr);
    if (m_sunTextureMemory) vkFreeMemory(m_engine.device(), m_sunTextureMemory, nullptr);
    if (m_particleTextureView) vkDestroyImageView(m_engine.device(), m_particleTextureView, nullptr);
    if (m_particleTexture) vkDestroyImage(m_engine.device(), m_particleTexture, nullptr);
    if (m_particleTextureMemory) vkFreeMemory(m_engine.device(), m_particleTextureMemory, nullptr);
}

void Textures::createImage(uint32_t w, uint32_t h, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8G8B8A8_SRGB;
    ici.extent = {w, h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_engine.device(), &ici, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_engine.device(), image, &memReq);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReq.size;
    ai.memoryTypeIndex = m_engine.findMemoryType(memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_engine.device(), &ai, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate texture memory");

    vkBindImageMemory(m_engine.device(), image, memory, 0);
}

void Textures::createImageView(VkImage image, VkFormat format, VkImageView& view) {
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = format;
    ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_engine.device(), &ci, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view");
}

void Textures::uploadTexture(VkImage image, uint32_t w, uint32_t h,
                              const std::vector<uint8_t>& pixels) {
    VkDeviceSize size = w * h * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    m_engine.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(m_engine.device(), stagingMemory, 0, size, 0, &data);
    memcpy(data, pixels.data(), size);
    vkUnmapMemory(m_engine.device(), stagingMemory);

    // Transition image to transfer dst layout
    VkCommandBuffer cmd = m_engine.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy staging to image
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {w, h, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition to shader read layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_engine.endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_engine.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_engine.device(), stagingMemory, nullptr);
}

void Textures::createSampler() {
    VkSamplerCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.maxLod = 1.0f;

    if (vkCreateSampler(m_engine.device(), &ci, nullptr, &m_sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler");
}

void Textures::createDescriptorSet() {
    // Descriptor set layout
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo lci{};
    lci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lci.bindingCount = 1;
    lci.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(m_engine.device(), &lci, nullptr,
                                    &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture descriptor set layout");

    // Pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2;

    VkDescriptorPoolCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.maxSets = 2;
    pci.poolSizeCount = 1;
    pci.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_engine.device(), &pci, nullptr, &m_descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture descriptor pool");

    // Allocate 2 sets
    std::vector<VkDescriptorSetLayout> layouts = {m_descriptorSetLayout, m_descriptorSetLayout};
    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = m_descriptorPool;
    ai.descriptorSetCount = 2;
    ai.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(2);
    if (vkAllocateDescriptorSets(m_engine.device(), &ai, sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate texture descriptor sets");

    m_sunDescriptorSet = sets[0];
    m_particleDescriptorSet = sets[1];

    // Write sun descriptor set
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_sunTextureView;
        imageInfo.sampler = m_sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_sunDescriptorSet;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_engine.device(), 1, &write, 0, nullptr);
    }

    // Write particle descriptor set
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_particleTextureView;
        imageInfo.sampler = m_sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_particleDescriptorSet;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_engine.device(), 1, &write, 0, nullptr);
    }
}

void Textures::createProceduralTextures() {
    createSampler();

    // 64×64 sun glow texture: soft radial gradient
    {
        uint32_t w = 64, h = 64;
        std::vector<uint8_t> pixels(w * h * 4);
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                float fx = (float)x / (float)w - 0.5f;
                float fy = (float)y / (float)h - 0.5f;
                float dist = std::sqrt(fx * fx + fy * fy);
                float glow = 1.0f - std::min(dist * 2.0f, 1.0f);
                glow = glow * glow; // quadratic falloff

                uint8_t val = (uint8_t)(glow * 255.0f);
                size_t idx = (y * w + x) * 4;
                pixels[idx + 0] = val; // R
                pixels[idx + 1] = val; // G
                pixels[idx + 2] = val; // B
                pixels[idx + 3] = val; // A
            }
        }

        createImage(w, h, m_sunTexture, m_sunTextureMemory);
        uploadTexture(m_sunTexture, w, h, pixels);
        createImageView(m_sunTexture, VK_FORMAT_R8G8B8A8_SRGB, m_sunTextureView);
    }

    // 16×16 particle glow texture
    {
        uint32_t w = 16, h = 16;
        std::vector<uint8_t> pixels(w * h * 4);
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                float fx = (float)x / (float)w - 0.5f;
                float fy = (float)y / (float)h - 0.5f;
                float dist = std::sqrt(fx * fx + fy * fy);
                float glow = 1.0f - std::min(dist * 2.0f, 1.0f);
                glow = glow * glow;

                uint8_t val = (uint8_t)(glow * 255.0f);
                size_t idx = (y * w + x) * 4;
                pixels[idx + 0] = val;
                pixels[idx + 1] = val;
                pixels[idx + 2] = val;
                pixels[idx + 3] = val;
            }
        }

        createImage(w, h, m_particleTexture, m_particleTextureMemory);
        uploadTexture(m_particleTexture, w, h, pixels);
        createImageView(m_particleTexture, VK_FORMAT_R8G8B8A8_SRGB, m_particleTextureView);
    }

    createDescriptorSet();
}
