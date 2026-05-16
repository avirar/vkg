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
    if (m_singBuffer) vkDestroyBuffer(m_engine.device(), m_singBuffer, nullptr);
    if (m_singMemory) vkFreeMemory(m_engine.device(), m_singMemory, nullptr);
    cleanupInitStaging();
}

void Compute::init(uint32_t particleCount) {
    m_particleCount = particleCount;
    m_maxParticles = particleCount;
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createParticleBuffers();
    createSingularityBuffer();
    createDescriptorSets();
}

void Compute::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding bindings[3]{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = 3;
    ci.pBindings = bindings;

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
    VkDeviceSize bufferSize = m_maxParticles * sizeof(Particle);
    m_particleBuffers.resize(2);
    m_particleBufferMemories.resize(2);

    for (int i = 0; i < 2; i++) {
        m_engine.createBuffer(bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_particleBuffers[i], m_particleBufferMemories[i]);
    }
}

void Compute::createSingularityBuffer() {
    VkDeviceSize size = sizeof(SingData) * 8;
    m_engine.createBuffer(size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_singBuffer, m_singMemory);
}

void Compute::createDescriptorSets() {
    // Pool: 2 sets × 3 bindings = 6 storage buffer descriptors
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 6;

    VkDescriptorPoolCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.maxSets = 2;
    pci.poolSizeCount = 1;
    pci.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(m_engine.device(), &pci, nullptr, &m_descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts = {m_descriptorSetLayout, m_descriptorSetLayout};
    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = m_descriptorPool;
    ai.descriptorSetCount = 2;
    ai.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(2);
    if (vkAllocateDescriptorSets(m_engine.device(), &ai, sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate compute descriptor sets");

    m_descriptorSetAB = sets[0];
    m_descriptorSetBA = sets[1];

    VkDeviceSize bufSize = m_maxParticles * sizeof(Particle);
    VkDeviceSize singSize = sizeof(SingData) * 8;

    // Write set AB: binding 0 -> buf[0], binding 1 -> buf[1], binding 2 -> singBuffer
    {
        VkDescriptorBufferInfo bi0{}, bi1{}, bi2{};
        bi0.buffer = m_particleBuffers[0]; bi0.offset = 0; bi0.range = bufSize;
        bi1.buffer = m_particleBuffers[1]; bi1.offset = 0; bi1.range = bufSize;
        bi2.buffer = m_singBuffer; bi2.offset = 0; bi2.range = singSize;

        VkWriteDescriptorSet writes[3]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_descriptorSetAB; writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &bi0;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_descriptorSetAB; writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &bi1;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_descriptorSetAB; writes[2].dstBinding = 2;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &bi2;

        vkUpdateDescriptorSets(m_engine.device(), 3, writes, 0, nullptr);
    }

    // Write set BA: binding 0 -> buf[1], binding 1 -> buf[0], binding 2 -> singBuffer
    {
        VkDescriptorBufferInfo bi0{}, bi1{}, bi2{};
        bi0.buffer = m_particleBuffers[1]; bi0.offset = 0; bi0.range = bufSize;
        bi1.buffer = m_particleBuffers[0]; bi1.offset = 0; bi1.range = bufSize;
        bi2.buffer = m_singBuffer; bi2.offset = 0; bi2.range = singSize;

        VkWriteDescriptorSet writes[3]{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_descriptorSetBA; writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &bi0;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_descriptorSetBA; writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &bi1;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_descriptorSetBA; writes[2].dstBinding = 2;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &bi2;

        vkUpdateDescriptorSets(m_engine.device(), 3, writes, 0, nullptr);
    }
}

void Compute::recordInitialParticles(VkCommandBuffer cmd) {
    VkDeviceSize bufferSize = m_maxParticles * sizeof(Particle);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    m_engine.createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(m_engine.device(), stagingMemory, 0, bufferSize, 0, &data);
    Particle* particles = static_cast<Particle*>(data);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-0.15f, 0.15f);

    for (uint32_t i = 0; i < m_maxParticles; i++) {
        particles[i].pos_x = dist(rng);
        particles[i].pos_y = dist(rng);
        particles[i].pos_z = dist(rng);
        particles[i].vel_x = 0.0f;
        particles[i].vel_y = 0.0f;
        particles[i].vel_z = 0.0f;
        particles[i].screen_x = 0.0f;
        particles[i].screen_y = 0.0f;
        particles[i].brightness = 0.0f;
        particles[i].hue = 0.0f;
    }
    vkUnmapMemory(m_engine.device(), stagingMemory);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_particleBuffers[0], 1, &copyRegion);
    vkCmdCopyBuffer(cmd, stagingBuffer, m_particleBuffers[1], 1, &copyRegion);

    m_initStagingBuf = stagingBuffer;
    m_initStagingMem = stagingMemory;
}

void Compute::update(float dt, uint32_t singCount, const SingData* singData,
                     float comX, float comY, float comZ,
                     float sinOrbit, float cosOrbit, float sinElev, float cosElev,
                     float arX, float arY) {
    m_push.dt = dt;
    m_push.gravity = 0.01f;
    m_push.damping = 0.982f;
    m_push.forceMult = m_push.gravity * dt * (1.0f + m_push.damping);
    m_push.particleCount = m_particleCount;
    m_push.singCount = std::min(singCount, 8u);
    m_push.comX = comX;
    m_push.comY = comY;
    m_push.comZ = comZ;
    m_push.sinOrbit = sinOrbit;
    m_push.cosOrbit = cosOrbit;
    m_push.sinElev = sinElev;
    m_push.cosElev = cosElev;
    m_push.cameraDist = 1.5f;
    m_push.cameraOffset = 0.6f;
    m_push.aspectRatioX = arX;
    m_push.aspectRatioY = arY;
    m_push.seed = ++m_seedCounter;
    m_push.debugMode = 0.0f;
    m_push.dbgScrX = 0.0f;
    m_push.dbgScrY = 0.0f;
    m_push.dbgBright = 0.0f;

    std::memcpy(m_singData, singData, sizeof(SingData) * std::min(singCount, 8u));
}

void Compute::dispatch(VkCommandBuffer cmd) {
    VkDescriptorSet set = (m_activeSet == 0) ? m_descriptorSetAB : m_descriptorSetBA;
    m_outputIndex = (m_activeSet == 0) ? 1 : 0;

    // Upload singularity positions inline (snapshotted at record time)
    vkCmdUpdateBuffer(cmd, m_singBuffer, 0, sizeof(m_singData), m_singData);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(ComputePushConstants), &m_push);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
                            0, 1, &set, 0, nullptr);

    uint32_t workgroups = (m_particleCount + 255) / 256;
    vkCmdDispatch(cmd, workgroups, 1, 1);

    VkBufferMemoryBarrier bufBarrier{};
    bufBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufBarrier.buffer = m_particleBuffers[m_outputIndex];
    bufBarrier.offset = 0;
    bufBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0,
        0, nullptr,
        1, &bufBarrier,
        0, nullptr);

    m_activeSet = 1 - m_activeSet;
}

void Compute::cleanupInitStaging() {
    if (m_initStagingBuf) {
        vkDestroyBuffer(m_engine.device(), m_initStagingBuf, nullptr);
        m_initStagingBuf = VK_NULL_HANDLE;
    }
    if (m_initStagingMem) {
        vkFreeMemory(m_engine.device(), m_initStagingMem, nullptr);
        m_initStagingMem = VK_NULL_HANDLE;
    }
}

void Compute::debugPlaceParticle(VkCommandBuffer cmd, float screenX, float screenY, float brightness) {
    m_outputIndex = (m_activeSet == 0) ? 1 : 0;
    m_activeSet = 1 - m_activeSet;

    Particle p{};
    p.screen_x = screenX;
    p.screen_y = screenY;
    p.brightness = brightness;

    vkCmdUpdateBuffer(cmd, m_particleBuffers[m_outputIndex], 0, sizeof(Particle), &p);

    VkBufferMemoryBarrier bufBarrier{};
    bufBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bufBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufBarrier.buffer = m_particleBuffers[m_outputIndex];
    bufBarrier.offset = 0;
    bufBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0, 0, nullptr, 1, &bufBarrier, 0, nullptr);
}
