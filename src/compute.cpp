#include "compute.h"
#include <cstring>
#include <random>
#include <cmath>
#include <iostream>

Compute::Compute(Engine& engine) : m_engine(engine) {
    vkGetPhysicalDeviceProperties(m_engine.physicalDevice(), &m_deviceProps);
}

Compute::~Compute() {
    vkDeviceWaitIdle(m_engine.device());
    if (m_pipeline) vkDestroyPipeline(m_engine.device(), m_pipeline, nullptr);
    if (m_pipelineLayout) vkDestroyPipelineLayout(m_engine.device(), m_pipelineLayout, nullptr);
    if (m_descriptorSetLayout) vkDestroyDescriptorSetLayout(m_engine.device(), m_descriptorSetLayout, nullptr);
    if (m_descriptorPool) vkDestroyDescriptorPool(m_engine.device(), m_descriptorPool, nullptr);
    for (size_t i = 0; i < m_particleBuffers.size(); i++) {
        vkDestroyBuffer(m_engine.device(), m_particleBuffers[i], nullptr);
        vkFreeMemory(m_engine.device(), m_particleBufferMemories[i], nullptr);
    }
}

void Compute::init(uint32_t particleCount) {
    m_particleCount = particleCount;
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createParticleBuffers();
    createDescriptorSets();
    initializeParticles();
}

void Compute::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = 1;
    ci.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(m_engine.device(), &ci, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute descriptor set layout");
}

void Compute::createPipelineLayout() {
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(ComputePushConstants);

    VkPipelineLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ci.setLayoutCount = 1;
    ci.pSetLayouts = &m_descriptorSetLayout;
    ci.pushConstantRangeCount = 1;
    ci.pPushConstantRanges = &pcr;

    if (vkCreatePipelineLayout(m_engine.device(), &ci, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout");
}

void Compute::createPipeline() {
    auto code = readFile("shaders/particle.comp.spv");
    VkShaderModule shaderModule = createShaderModule(m_engine.device(), code);

    VkPipelineShaderStageCreateInfo ssi{};
    ssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ssi.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    ssi.module = shaderModule;
    ssi.pName = "main";

    VkComputePipelineCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.stage = ssi;
    ci.layout = m_pipelineLayout;

    if (vkCreateComputePipelines(m_engine.device(), VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline");

    vkDestroyShaderModule(m_engine.device(), shaderModule, nullptr);
}

void Compute::createParticleBuffers() {
    VkDeviceSize bufferSize = MAX_PARTICLES * sizeof(Particle);
    m_particleBuffers.resize(2);
    m_particleBufferMemories.resize(2);

    for (int i = 0; i < 2; i++) {
        m_engine.createBuffer(bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_particleBuffers[i], m_particleBufferMemories[i]);
    }
}

void Compute::createDescriptorSets() {
    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 2;

    VkDescriptorPoolCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.maxSets = 2;
    pci.poolSizeCount = 1;
    pci.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_engine.device(), &pci, nullptr, &m_descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute descriptor pool");

    // Allocate 2 descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(2, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = m_descriptorPool;
    ai.descriptorSetCount = 2;
    ai.pSetLayouts = layouts.data();

    m_descriptorSets.resize(2);
    if (vkAllocateDescriptorSets(m_engine.device(), &ai, m_descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate compute descriptor sets");

    // Write descriptor sets
    for (int i = 0; i < 2; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_particleBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = MAX_PARTICLES * sizeof(Particle);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_descriptorSets[i];
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_engine.device(), 1, &write, 0, nullptr);
    }
}

void Compute::initializeParticles() {
    VkDeviceSize bufferSize = MAX_PARTICLES * sizeof(Particle);

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    m_engine.createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    // Fill with initial data (matches first init: cube distribution [-0.15, 0.15])
    void* data;
    vkMapMemory(m_engine.device(), stagingMemory, 0, bufferSize, 0, &data);
    Particle* particles = static_cast<Particle*>(data);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-0.15f, 0.15f);

    for (uint32_t i = 0; i < MAX_PARTICLES; i++) {
        particles[i].pos_x = dist(rng);
        particles[i].pos_y = dist(rng);
        particles[i].pos_z = dist(rng);
        particles[i].vel_x = 0.0f;
        particles[i].vel_y = 0.0f;
        particles[i].vel_z = 0.0f;
        particles[i].screen_x = 0.0f;
        particles[i].screen_y = 0.0f;
        particles[i].brightness = 0.0f;
        particles[i]._pad = 0.0f;
    }
    vkUnmapMemory(m_engine.device(), stagingMemory);

    // Copy staging to both device buffers
    VkCommandBuffer cmd = m_engine.beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_particleBuffers[0], 1, &copyRegion);
    vkCmdCopyBuffer(cmd, stagingBuffer, m_particleBuffers[1], 1, &copyRegion);
    m_engine.endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_engine.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_engine.device(), stagingMemory, nullptr);
}

void Compute::update(float dt, float sx, float sy, float sz,
                     float sinRot, float cosRot, float arX, float arY) {
    // The dispatch happens in dispatch() below
    m_push.dt = dt;
    m_push.gravity = 0.01f;
    m_push.damping = 0.982f;
    m_push.particleCount = m_particleCount;
    m_push.singularityX = sx;
    m_push.singularityY = sy;
    m_push.singularityZ = sz;
    m_push.sinRot = sinRot;
    m_push.cosRot = cosRot;
    m_push.cameraDist = 1.5f;
    m_push.cameraOffset = 0.0f;
    m_push.aspectRatioX = arX;
    m_push.aspectRatioY = arY;
}

void Compute::dispatch(VkCommandBuffer cmd) {
    // Buffer ping-pong: read from outputIndex (previous output), write to !outputIndex
    uint32_t readIdx = m_outputIndex;
    m_outputIndex = 1 - m_outputIndex; // toggle for next frame

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    // Push constants
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(ComputePushConstants), &m_push);

    // Bind descriptor set pointing to current read buffer
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
                            0, 1, &m_descriptorSets[readIdx], 0, nullptr);

    // Dispatch
    uint32_t workgroups = (m_particleCount + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);

    // Barrier: ensure compute writes are visible to vertex shader reads
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr);
}
