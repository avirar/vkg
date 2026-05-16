#include "render.h"
#include <array>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <vector>

Renderer::Renderer(Engine& engine) : m_engine(engine) {
    createCommandBuffers();
    createSunVertexBuffer();
    createSunIndexBuffer();
    initScreenshotBuffer();
}

void Renderer::createPipelines() {
    if (!m_textures) return;
    createGraphicsPipeline();
    createSunPipeline();
}

Renderer::~Renderer() {
    vkDestroyPipeline(m_engine.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_engine.device(), m_graphicsPipelineLayout, nullptr);
    vkDestroyPipeline(m_engine.device(), m_sunPipeline, nullptr);
    vkDestroyPipelineLayout(m_engine.device(), m_sunPipelineLayout, nullptr);
    vkDestroyBuffer(m_engine.device(), m_sunVertexBuffer, nullptr);
    vkFreeMemory(m_engine.device(), m_sunVertexMemory, nullptr);
    vkDestroyBuffer(m_engine.device(), m_sunIndexBuffer, nullptr);
    vkFreeMemory(m_engine.device(), m_sunIndexMemory, nullptr);
    if (m_ssBuffer) vkDestroyBuffer(m_engine.device(), m_ssBuffer, nullptr);
    if (m_ssMemory) vkFreeMemory(m_engine.device(), m_ssMemory, nullptr);
    vkFreeCommandBuffers(m_engine.device(), m_engine.commandPool(),
                         (uint32_t)m_commandBuffers.size(), m_commandBuffers.data());
}

void Renderer::createCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = m_engine.commandPool();
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(m_engine.device(), &ai, m_commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
}

void Renderer::createSunVertexBuffer() {
    // Quad vertices: triangle strip covering [-1,1] x [-1,1]
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    VkDeviceSize size = sizeof(vertices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    m_engine.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(m_engine.device(), stagingMemory, 0, size, 0, &data);
    memcpy(data, vertices, size);
    vkUnmapMemory(m_engine.device(), stagingMemory);

    m_engine.createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_sunVertexBuffer, m_sunVertexMemory);

    VkCommandBuffer cmd = m_engine.beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_sunVertexBuffer, 1, &copyRegion);
    m_engine.endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_engine.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_engine.device(), stagingMemory, nullptr);
}

void Renderer::createSunIndexBuffer() {
    // Two triangles covering [-1,1] x [-1,1] quad
    // Vertices: 0=(-1,-1), 1=(1,-1), 2=(-1,1), 3=(1,1)
    uint16_t indices[] = {0, 1, 2, 2, 1, 3};
    VkDeviceSize size = sizeof(indices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    m_engine.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(m_engine.device(), stagingMemory, 0, size, 0, &data);
    memcpy(data, indices, size);
    vkUnmapMemory(m_engine.device(), stagingMemory);

    m_engine.createBuffer(size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_sunIndexBuffer, m_sunIndexMemory);

    VkCommandBuffer cmd = m_engine.beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_sunIndexBuffer, 1, &copyRegion);
    m_engine.endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_engine.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_engine.device(), stagingMemory, nullptr);
}

void Renderer::createSunPipeline() {
    auto vertCode = readFile("shaders/sun.vert.spv");
    auto fragCode = readFile("shaders/sun.frag.spv");
    VkShaderModule vertMod = createShaderModule(m_engine.device(), vertCode);
    VkShaderModule fragMod = createShaderModule(m_engine.device(), fragCode);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = 2 * sizeof(float);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDesc{};
    attrDesc.binding = 0;
    attrDesc.location = 0;
    attrDesc.format = VK_FORMAT_R32G32_SFLOAT;
    attrDesc.offset = 0;

    VkPipelineVertexInputStateCreateInfo vis{};
    vis.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount = 1;
    vis.pVertexBindingDescriptions = &bindDesc;
    vis.vertexAttributeDescriptionCount = 1;
    vis.pVertexAttributeDescriptions = &attrDesc;

    VkPipelineInputAssemblyStateCreateInfo ias{};
    ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vs{};
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs.viewportCount = 1;
    vs.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    // Additive blending for sun layers like original
    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbs{};
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1;
    cbs.pAttachments = &cba;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dsc{};
    dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsc.dynamicStateCount = (uint32_t)dynamicStates.size();
    dsc.pDynamicStates = dynamicStates.data();

    // Push constants for sun position/scale/alpha
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.size = sizeof(SunPushConstants);

    VkDescriptorSetLayout setLayouts[1] = { m_textures->descriptorSetLayout() };

    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = setLayouts;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    if (vkCreatePipelineLayout(m_engine.device(), &plci, nullptr, &m_sunPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sun pipeline layout");

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vis;
    pci.pInputAssemblyState = &ias;
    pci.pViewportState = &vs;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms;
    pci.pDepthStencilState = &ds;
    pci.pColorBlendState = &cbs;
    pci.pDynamicState = &dsc;
    pci.layout = m_sunPipelineLayout;
    pci.renderPass = m_engine.renderPass();
    pci.subpass = 0;

    if (vkCreateGraphicsPipelines(m_engine.device(), VK_NULL_HANDLE, 1, &pci, nullptr,
                                  &m_sunPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sun pipeline");

    vkDestroyShaderModule(m_engine.device(), vertMod, nullptr);
    vkDestroyShaderModule(m_engine.device(), fragMod, nullptr);
}

void Renderer::createGraphicsPipeline() {
    auto vertCode = readFile("shaders/quad.vert.spv");
    auto fragCode = readFile("shaders/quad.frag.spv");
    VkShaderModule vertMod = createShaderModule(m_engine.device(), vertCode);
    VkShaderModule fragMod = createShaderModule(m_engine.device(), fragCode);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(Particle);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[3]{};
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset = offsetof(Particle, screen_x);

    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32_SFLOAT;
    attrDescs[1].offset = offsetof(Particle, brightness);

    attrDescs[2].binding = 0;
    attrDescs[2].location = 2;
    attrDescs[2].format = VK_FORMAT_R32_SFLOAT;
    attrDescs[2].offset = offsetof(Particle, hue);

    VkPipelineVertexInputStateCreateInfo vis{};
    vis.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount = 1;
    vis.pVertexBindingDescriptions = &bindDesc;
    vis.vertexAttributeDescriptionCount = 3;
    vis.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo ias{};
    ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    VkPipelineViewportStateCreateInfo vs{};
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs.viewportCount = 1;
    vs.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbs{};
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1;
    cbs.pAttachments = &cba;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dsc{};
    dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsc.dynamicStateCount = (uint32_t)dynamicStates.size();
    dsc.pDynamicStates = dynamicStates.data();

    // Pipeline layout with texture descriptor set
    VkDescriptorSetLayout setLayouts[1] = { m_textures->descriptorSetLayout() };

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.size = sizeof(ParticlePushConstants);

    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = setLayouts;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;

    if (vkCreatePipelineLayout(m_engine.device(), &plci, nullptr, &m_graphicsPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline layout");

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vis;
    pci.pInputAssemblyState = &ias;
    pci.pViewportState = &vs;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms;
    pci.pDepthStencilState = &ds;
    pci.pColorBlendState = &cbs;
    pci.pDynamicState = &dsc;
    pci.layout = m_graphicsPipelineLayout;
    pci.renderPass = m_engine.renderPass();
    pci.subpass = 0;

    if (vkCreateGraphicsPipelines(m_engine.device(), VK_NULL_HANDLE, 1, &pci, nullptr,
                                  &m_graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");

    vkDestroyShaderModule(m_engine.device(), vertMod, nullptr);
    vkDestroyShaderModule(m_engine.device(), fragMod, nullptr);
}

void Renderer::drawFrame(float sinOrbit, float cosOrbit,
                          float sinElev, float cosElev,
                          float singX, float singY, float singZ,
                          float aspectX, float aspectY,
                          float sunPulse) {
    uint32_t frameIdx = m_engine.currentFrame();
    VkCommandBuffer cmd = m_commandBuffers[frameIdx];

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &bi);

    // Dispatch compute shader
    if (m_debugMode) {
        if (m_compute) m_compute->debugPlaceParticle(cmd, 0.5f, 0.3f, 1.0f);
    } else if (m_compute) {
        m_compute->dispatch(cmd);
    }

    // Begin render pass
    VkRenderPassBeginInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = m_engine.renderPass();
    rpi.framebuffer = m_engine.currentFramebuffer();
    rpi.renderArea.extent = m_engine.extent();
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    rpi.clearValueCount = 1;
    rpi.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpi, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport/scissor (same for both pipelines)
    VkViewport vp{};
    vp.x = 0; vp.y = 0;
    vp.width = (float)m_engine.extent().width;
    vp.height = (float)m_engine.extent().height;
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent = m_engine.extent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // --- Draw sun (7 concentric layers) ---
    {
        // Project singularity to screen with camera orbit + elevation
        float camDist = 1.5f;
        float camOff = 0.6f;
        float ryX = cosOrbit * singX - sinOrbit * singZ;
        float ryZ = sinOrbit * singX + cosOrbit * singZ;
        float depth = cosElev * ryZ + sinElev * singY;
        float elevY = cosElev * singY - sinElev * ryZ;
        float persp = camDist / (depth + camOff);
        float sunScrX = ryX * persp * aspectX;
        float sunScrY = elevY * persp * aspectY;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sunPipeline);

        if (m_textures) {
            VkDescriptorSet sunSet = m_textures->sunDescriptorSet();
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_sunPipelineLayout, 0, 1, &sunSet, 0, nullptr);
        }

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_sunVertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, m_sunIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // 7 layers matching original (glg.c:2377-2485): 0.20, 0.08, 0.04, 0.02, 0.02, 0.01, 0.01
        float baseSize = 0.20f;
        SunPushConstants spc{};
        spc.centerX = sunScrX;
        spc.centerY = sunScrY;

        // Layer 1: 0.20
        float layerSize = baseSize;
        spc.scaleX = layerSize * aspectX;
        spc.scaleY = layerSize;
        spc.alpha = 0.25f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 2: 0.08
        layerSize = baseSize * 0.4f;
        spc.scaleX = layerSize * aspectX;
        spc.scaleY = layerSize;
        spc.alpha = 0.35f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 3: 0.04
        layerSize *= 0.5f;
        spc.scaleX = layerSize * aspectX;
        spc.scaleY = layerSize;
        spc.alpha = 0.45f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 4: 0.02
        layerSize *= 0.5f;
        spc.scaleX = layerSize * aspectX;
        spc.scaleY = layerSize;
        spc.alpha = 0.55f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 5: same size
        spc.alpha = 0.65f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 6: 0.01
        layerSize *= 0.5f;
        spc.scaleX = layerSize * aspectX;
        spc.scaleY = layerSize;
        spc.alpha = 0.80f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

        // Layer 7: same size
        spc.alpha = 1.0f * sunPulse;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
    }

    // --- Draw particles ---
    if (!m_debugMode && m_compute && m_compute->particleCount() > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        ParticlePushConstants ppc = m_ppc;
        ppc.viewportHeight = (float)m_engine.extent().height;
        ppc.aspectY = aspectY;
        ppc.pointSizeMult = m_debugMode ? 3.0f : m_pointScale;
        vkCmdPushConstants(cmd, m_graphicsPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ppc), &ppc);

        if (m_textures) {
            VkDescriptorSet partSet = m_textures->particleDescriptorSet();
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_graphicsPipelineLayout, 0, 1, &partSet, 0, nullptr);
        }

        VkDeviceSize offset = 0;
        VkBuffer particleBuf = m_compute->outputBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &particleBuf, &offset);
        vkCmdDraw(cmd, m_compute->particleCount(), 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);

    if (!m_ssCaptured) {
        VkImage srcImage = m_engine.currentSwapchainImage();
        VkExtent2D ext = m_engine.extent();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {ext.width, ext.height, 1};

        vkCmdCopyImageToBuffer(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               m_ssBuffer, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    vkEndCommandBuffer(cmd);

    VkSemaphore waitSem = m_engine.imageAvailableSemaphore();
    VkSemaphore signalSem = m_engine.renderFinishedSemaphore();
    VkFence fence = m_engine.currentFence();

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &waitSem;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &signalSem;

    vkResetFences(m_engine.device(), 1, &fence);
    VkResult result = vkQueueSubmit(m_engine.graphicsQueue(), 1, &si, fence);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer: code " + std::to_string(result));

    if (m_debugMode)
        debugDump(fence);
    else
        saveScreenshot(fence);
}

void Renderer::setParticleColors(const Config& cfg) {
    m_ppc = {};
    if (cfg.hypercolorMode == "brightness")
        m_ppc.mode = 2u;
    else if (cfg.hypercolorMode == "color")
        m_ppc.mode = 1u;
    else
        m_ppc.mode = 0u;  // "off" or unknown
    m_ppc.hyperIntensity = cfg.hyperIntensity;
    m_ppc.loR = cfg.hyperLoR; m_ppc.loG = cfg.hyperLoG; m_ppc.loB = cfg.hyperLoB;
    m_ppc.hiR = cfg.hyperHiR; m_ppc.hiG = cfg.hyperHiG; m_ppc.hiB = cfg.hyperHiB;
}

void Renderer::debugDump(VkFence fence) {
    if (m_ssCaptured) return;
    vkWaitForFences(m_engine.device(), 1, &fence, VK_TRUE, UINT64_MAX);

    VkExtent2D extent = m_engine.extent();
    VkDeviceSize size = extent.width * extent.height * 4;
    void* data;
    vkMapMemory(m_engine.device(), m_ssMemory, 0, size, 0, &data);
    uint8_t* pixels = (uint8_t*)data;

    // Summary: count pixels per brightness level
    int counts[6] = {0,0,0,0,0,0};  // 0, 1-25, 26-50, 51-100, 101-180, 181-255
    int maxVal = 0, maxX = 0, maxY = 0;
    for (uint32_t y = 0; y < extent.height; y++) {
        for (uint32_t x = 0; x < extent.width; x++) {
            uint32_t i = y * extent.width + x;
            uint8_t r = pixels[i * 4 + 2];
            uint8_t g = pixels[i * 4 + 1];
            uint8_t b = pixels[i * 4 + 0];
            uint8_t m = r;
            if (g > m) m = g;
            if (b > m) m = b;
            if (m > maxVal) { maxVal = m; maxX = x; maxY = y; }
            if (m == 0) counts[0]++;
            else if (m <= 25) counts[1]++;
            else if (m <= 50) counts[2]++;
            else if (m <= 100) counts[3]++;
            else if (m <= 180) counts[4]++;
            else counts[5]++;
        }
    }
    printf("\n=== Debug: %ux%u ===\n", extent.width, extent.height);
    printf("Pixels:  0=%d  .(1-25)=%d  :(26-50)=%d  +(51-100)=%d  *(101-180)=%d  #(181+)=%d\n",
           counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
    printf("Brightest: (%d,%d) R=%u G=%u B=%u\n\n", maxX, maxY,
           pixels[(maxY * extent.width + maxX) * 4 + 2],
           pixels[(maxY * extent.width + maxX) * 4 + 1],
           pixels[(maxY * extent.width + maxX) * 4 + 0]);

    // ASCII grid (full screen)
    printf("=== ASCII grid (legend: .=1-25 :=26-50 +=51-100 *=101-180 #=181+) ===\n");
    for (uint32_t y = 0; y < extent.height; y++) {
        for (uint32_t x = 0; x < extent.width; x++) {
            uint32_t i = y * extent.width + x;
            uint8_t r = pixels[i * 4 + 2];
            uint8_t g = pixels[i * 4 + 1];
            uint8_t b = pixels[i * 4 + 0];
            uint8_t maxC = r;
            if (g > maxC) maxC = g;
            if (b > maxC) maxC = b;
            char c;
            if (maxC == 0)      c = ' ';
            else if (maxC <= 25)  c = '.';
            else if (maxC <= 50)  c = ':';
            else if (maxC <= 100) c = '+';
            else if (maxC <= 180) c = '*';
            else                  c = '#';
            putchar(c);
        }
        putchar('\n');
    }

    // Numeric dump of center 20x20 region
    printf("\n=== Center 20x20 numeric (max(R,G,B) per pixel) ===\n");
    int cy = extent.height / 2;
    int cx = extent.width / 2;
    for (int y = cy - 10; y < cy + 10; y++) {
        if (y < 0 || y >= (int)extent.height) {
            printf("---\n");
            continue;
        }
        for (int x = cx - 10; x < cx + 10; x++) {
            if (x < 0 || x >= (int)extent.width) {
                printf("---");
                continue;
            }
            uint32_t i = y * extent.width + x;
            uint8_t r = pixels[i * 4 + 2];
            uint8_t g = pixels[i * 4 + 1];
            uint8_t b = pixels[i * 4 + 0];
            uint8_t m = r;
            if (g > m) m = g;
            if (b > m) m = b;
            printf("%3u ", m);
        }
        printf("\n");
    }

    // All non-zero pixels outside center region (to find debug particle)
    printf("\n=== Non-zero pixels outside center 20x20 ===\n");
    int found = 0;
    for (uint32_t y = 0; y < extent.height && found < 50; y++) {
        for (uint32_t x = 0; x < extent.width && found < 50; x++) {
            if (x >= (uint32_t)(cx - 10) && x <= (uint32_t)(cx + 9) &&
                y >= (uint32_t)(cy - 10) && y <= (uint32_t)(cy + 9))
                continue;
            uint32_t i = y * extent.width + x;
            uint8_t r = pixels[i * 4 + 2];
            uint8_t g = pixels[i * 4 + 1];
            uint8_t b = pixels[i * 4 + 0];
            if (r > 0 || g > 0 || b > 0) {
                printf("  (%u,%u) R=%u G=%u B=%u\n", x, y, r, g, b);
                found++;
            }
        }
    }
    if (found == 0) printf("  (none)\n");

    vkUnmapMemory(m_engine.device(), m_ssMemory);
    m_ssCaptured = true;
}

void Renderer::initScreenshotBuffer() {
    VkExtent2D extent = m_engine.extent();
    VkDeviceSize size = extent.width * extent.height * 4;
    m_engine.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_ssBuffer, m_ssMemory);
}

void Renderer::saveScreenshot(VkFence fence) {
    if (m_ssCaptured) return;
    vkWaitForFences(m_engine.device(), 1, &fence, VK_TRUE, UINT64_MAX);

    VkExtent2D extent = m_engine.extent();
    VkDeviceSize size = extent.width * extent.height * 4;
    void* data;
    vkMapMemory(m_engine.device(), m_ssMemory, 0, size, 0, &data);

    uint8_t* pixels = (uint8_t*)data;
    std::vector<uint8_t> rgb(extent.width * extent.height * 3);
    for (uint32_t i = 0; i < extent.width * extent.height; i++) {
        rgb[i * 3 + 0] = pixels[i * 4 + 2];
        rgb[i * 3 + 1] = pixels[i * 4 + 1];
        rgb[i * 3 + 2] = pixels[i * 4 + 0];
    }

    FILE* f = fopen("/tmp/vkg_capture.ppm", "wb");
    if (f) {
        fprintf(f, "P6\n%u %u\n255\n", extent.width, extent.height);
        fwrite(rgb.data(), 1, rgb.size(), f);
        fclose(f);
    }

    vkUnmapMemory(m_engine.device(), m_ssMemory);
    m_ssCaptured = true;
}
