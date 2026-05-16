#include "render.h"
#include <array>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

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
    createOsdPipeline();
}

Renderer::~Renderer() {
    cleanupSunInitStaging();
    vkDestroyPipeline(m_engine.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_engine.device(), m_graphicsPipelineLayout, nullptr);
    vkDestroyPipeline(m_engine.device(), m_sunPipeline, nullptr);
    vkDestroyPipelineLayout(m_engine.device(), m_sunPipelineLayout, nullptr);
    vkDestroyBuffer(m_engine.device(), m_sunVertexBuffer, nullptr);
    vkFreeMemory(m_engine.device(), m_sunVertexMemory, nullptr);
    vkDestroyBuffer(m_engine.device(), m_sunIndexBuffer, nullptr);
    vkFreeMemory(m_engine.device(), m_sunIndexMemory, nullptr);
    if (m_osdPipeline) vkDestroyPipeline(m_engine.device(), m_osdPipeline, nullptr);
    if (m_osdPipelineLayout) vkDestroyPipelineLayout(m_engine.device(), m_osdPipelineLayout, nullptr);
    if (m_osdVertexBuffer) vkDestroyBuffer(m_engine.device(), m_osdVertexBuffer, nullptr);
    if (m_osdVertexMemory) vkFreeMemory(m_engine.device(), m_osdVertexMemory, nullptr);
    if (m_osdIndexBuffer) vkDestroyBuffer(m_engine.device(), m_osdIndexBuffer, nullptr);
    if (m_osdIndexMemory) vkFreeMemory(m_engine.device(), m_osdIndexMemory, nullptr);
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
    VkDeviceSize size = 8 * sizeof(float);
    m_engine.createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_sunVertexBuffer, m_sunVertexMemory);
}

void Renderer::createSunIndexBuffer() {
    VkDeviceSize size = 6 * sizeof(uint16_t);
    m_engine.createBuffer(size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_sunIndexBuffer, m_sunIndexMemory);
}

void Renderer::recordSunGeometryInit(VkCommandBuffer cmd) {
    float vertices[] = { -1.0f,-1.0f, 1.0f,-1.0f, -1.0f,1.0f, 1.0f,1.0f };
    VkDeviceSize vSize = sizeof(vertices);

    m_engine.createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_sunInitStagingVB, m_sunInitStagingVBMem);
    void* data;
    vkMapMemory(m_engine.device(), m_sunInitStagingVBMem, 0, vSize, 0, &data);
    memcpy(data, vertices, vSize);
    vkUnmapMemory(m_engine.device(), m_sunInitStagingVBMem);
    VkBufferCopy vCopy{};
    vCopy.size = vSize;
    vkCmdCopyBuffer(cmd, m_sunInitStagingVB, m_sunVertexBuffer, 1, &vCopy);

    uint16_t indices[] = { 0,1,2, 2,1,3 };
    VkDeviceSize iSize = sizeof(indices);

    m_engine.createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_sunInitStagingIB, m_sunInitStagingIBMem);
    vkMapMemory(m_engine.device(), m_sunInitStagingIBMem, 0, iSize, 0, &data);
    memcpy(data, indices, iSize);
    vkUnmapMemory(m_engine.device(), m_sunInitStagingIBMem);
    VkBufferCopy iCopy{};
    iCopy.size = iSize;
    vkCmdCopyBuffer(cmd, m_sunInitStagingIB, m_sunIndexBuffer, 1, &iCopy);
}

void Renderer::cleanupSunInitStaging() {
    if (m_sunInitStagingVB) {
        vkDestroyBuffer(m_engine.device(), m_sunInitStagingVB, nullptr);
        m_sunInitStagingVB = VK_NULL_HANDLE;
    }
    if (m_sunInitStagingVBMem) {
        vkFreeMemory(m_engine.device(), m_sunInitStagingVBMem, nullptr);
        m_sunInitStagingVBMem = VK_NULL_HANDLE;
    }
    if (m_sunInitStagingIB) {
        vkDestroyBuffer(m_engine.device(), m_sunInitStagingIB, nullptr);
        m_sunInitStagingIB = VK_NULL_HANDLE;
    }
    if (m_sunInitStagingIBMem) {
        vkFreeMemory(m_engine.device(), m_sunInitStagingIBMem, nullptr);
        m_sunInitStagingIBMem = VK_NULL_HANDLE;
    }
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
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
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

void Renderer::createOsdPipeline() {
    // OSD pipeline uses stb_easy_font vertex format (16-byte interleaved pos+color)
    // Not created here -- done lazily on first drawOsd call
}

void Renderer::drawFrame(const SimState& state, float aspectX, float aspectY) {
    uint32_t frameIdx = m_engine.currentFrame() % (uint32_t)m_commandBuffers.size();
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

    // --- Draw suns (one per singularity) ---
    {
        float camDist = 1.5f;
        float camOff = 0.6f;
        float orbitRad = state.orbitAngle * 3.14159265f / 180.0f;
        float cosOrbit = std::cos(orbitRad);
        float sinOrbit = std::sin(orbitRad);
        float elevAngle = std::sin(state.wobblePhase * 0.7f) * 5.0f * 3.14159265f / 180.0f;
        elevAngle += std::sin(state.wobblePhase * 1.3f + 1.0f) * 2.0f * 3.14159265f / 180.0f;
        float cosElev = std::cos(elevAngle);
        float sinElev = std::sin(elevAngle);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sunPipeline);

        if (m_textures) {
            VkDescriptorSet sunSet = m_textures->sunDescriptorSet();
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_sunPipelineLayout, 0, 1, &sunSet, 0, nullptr);
        }

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_sunVertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, m_sunIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

        for (uint32_t i = 0; i < state.singularityCount; i++) {
            const auto& s = state.singularities[i];
            float ryX = cosOrbit * s.x - sinOrbit * s.z;
            float ryZ = sinOrbit * s.x + cosOrbit * s.z;
            float depth = cosElev * ryZ + sinElev * s.y;
            float elevY = cosElev * s.y - sinElev * ryZ;
            float persp = camDist / (depth + camOff);
            float sunScrX = ryX * persp * state.aspectRatioX;
            float sunScrY = elevY * persp * state.aspectRatioY;

            SunPushConstants spc{};
            spc.centerX = sunScrX;
            spc.centerY = sunScrY;
            spc.aspectX = state.aspectRatioX;
            spc.sunPulse = state.sunPulse * (1.0f + 0.3f * std::sin(state.wobblePhase + s.pulsePhase));
            spc.layerScales[0] = 0.20f; spc.layerAlphas[0] = 0.25f;
            spc.layerScales[1] = 0.08f; spc.layerAlphas[1] = 0.35f;
            spc.layerScales[2] = 0.04f; spc.layerAlphas[2] = 0.45f;
            spc.layerScales[3] = 0.02f; spc.layerAlphas[3] = 0.55f;
            spc.layerScales[4] = 0.02f; spc.layerAlphas[4] = 0.65f;
            spc.layerScales[5] = 0.01f; spc.layerAlphas[5] = 0.80f;
            spc.layerScales[6] = 0.01f; spc.layerAlphas[6] = 1.00f;
            vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(spc), &spc);
            vkCmdDrawIndexed(cmd, 6, 7, 0, 0, 0);
        }
    }

    // --- Draw particles ---
    if (!m_debugMode && m_compute && m_compute->particleCount() > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        ParticlePushConstants ppc = m_ppc;
        ppc.viewportHeight = (float)m_engine.extent().height;
        ppc.aspectY = state.aspectRatioY;
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
        m_ppc.mode = 0u;
    m_ppc.loR = cfg.hyperLoR; m_ppc.loG = cfg.hyperLoG; m_ppc.loB = cfg.hyperLoB;
    m_ppc.hiR = cfg.hyperHiR; m_ppc.hiG = cfg.hyperHiG; m_ppc.hiB = cfg.hyperHiB;
}

void Renderer::debugDump(VkFence fence) {
    if (m_ssCaptured) return;
    vkWaitForFences(m_engine.device(), 1, &fence, VK_TRUE, UINT64_MAX);

    VkExtent2D extent = m_engine.extent();
    VkFormat fmt = m_engine.swapChainFormat();
    bool isFloat16 = (fmt == VK_FORMAT_R16G16B16A16_SFLOAT);
    int bpp = isFloat16 ? 8 : 4;
    VkDeviceSize size = extent.width * extent.height * bpp;
    void* data;
    vkMapMemory(m_engine.device(), m_ssMemory, 0, size, 0, &data);

    std::vector<uint8_t> converted;
    uint8_t* pixels;
    if (isFloat16) {
        auto h2f = [](uint16_t h) -> float {
            uint32_t sign = (h & 0x8000u) << 16;
            int e = (h >> 10) & 0x1F;
            uint32_t m = h & 0x3FFu;
            if (e == 0) {
                if (m == 0) { uint32_t f = sign; return *(float*)&f; }
                e = 1; while ((m & 0x400u) == 0) { m <<= 1; e--; }
                m &= 0x3FFu;
            } else if (e == 31) {
                uint32_t f = sign | 0x7F800000u | (m << 13);
                return *(float*)&f;
            }
            e = e - 15 + 127;
            uint32_t f = sign | ((uint32_t)e << 23) | (m << 13);
            return *(float*)&f;
        };
        converted.resize(extent.width * extent.height * 4);
        uint16_t* p16 = (uint16_t*)data;
        for (uint32_t i = 0; i < extent.width * extent.height; i++) {
            uint32_t base = i * 4;
            float fr = h2f(p16[base + 0]);
            float fg = h2f(p16[base + 1]);
            float fb = h2f(p16[base + 2]);
            auto to8 = [](float f) { return (uint8_t)std::clamp(f * 255.0f, 0.0f, 255.0f); };
            converted[i * 4 + 2] = to8(fr);
            converted[i * 4 + 1] = to8(fg);
            converted[i * 4 + 0] = to8(fb);
        }
        pixels = converted.data();
    } else {
        pixels = (uint8_t*)data;
    }

    int counts[6] = {0,0,0,0,0,0};
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

    printf("\n=== Center 20x20 numeric (max(R,G,B) per pixel) ===\n");
    int cy = extent.height / 2;
    int cx = extent.width / 2;
    for (int y = cy - 10; y < cy + 10; y++) {
        if (y < 0 || y >= (int)extent.height) { printf("---\n"); continue; }
        for (int x = cx - 10; x < cx + 10; x++) {
            if (x < 0 || x >= (int)extent.width) { printf("---"); continue; }
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
    VkFormat fmt = m_engine.swapChainFormat();
    int bpp = 4;
    if (fmt == VK_FORMAT_R16G16B16A16_SFLOAT) bpp = 8;
    VkDeviceSize size = extent.width * extent.height * bpp;
    m_engine.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_ssBuffer, m_ssMemory);
}

void Renderer::saveScreenshot(VkFence fence) {
    if (m_ssCaptured) return;
    vkWaitForFences(m_engine.device(), 1, &fence, VK_TRUE, UINT64_MAX);

    VkExtent2D extent = m_engine.extent();
    VkFormat fmt = m_engine.swapChainFormat();
    bool isFloat16 = (fmt == VK_FORMAT_R16G16B16A16_SFLOAT);
    int bpp = isFloat16 ? 8 : 4;
    VkDeviceSize size = extent.width * extent.height * bpp;
    void* data;
    vkMapMemory(m_engine.device(), m_ssMemory, 0, size, 0, &data);

    std::vector<uint8_t> rgb(extent.width * extent.height * 3);
    if (isFloat16) {
        auto h2f = [](uint16_t h) -> float {
            uint32_t sign = (h & 0x8000u) << 16;
            int e = (h >> 10) & 0x1F;
            uint32_t m = h & 0x3FFu;
            if (e == 0) {
                if (m == 0) { uint32_t f = sign; return *(float*)&f; }
                e = 1; while ((m & 0x400u) == 0) { m <<= 1; e--; }
                m &= 0x3FFu;
            } else if (e == 31) {
                uint32_t f = sign | 0x7F800000u | (m << 13);
                return *(float*)&f;
            }
            e = e - 15 + 127;
            uint32_t f = sign | ((uint32_t)e << 23) | (m << 13);
            return *(float*)&f;
        };
        uint16_t* p16 = (uint16_t*)data;
        for (uint32_t i = 0; i < extent.width * extent.height; i++) {
            uint32_t base = i * 4;
            auto to8 = [](float f) { return (uint8_t)std::clamp(f * 255.0f, 0.0f, 255.0f); };
            rgb[i * 3 + 0] = to8(h2f(p16[base + 0]));
            rgb[i * 3 + 1] = to8(h2f(p16[base + 1]));
            rgb[i * 3 + 2] = to8(h2f(p16[base + 2]));
        }
    } else {
        uint8_t* pixels = (uint8_t*)data;
        for (uint32_t i = 0; i < extent.width * extent.height; i++) {
            rgb[i * 3 + 0] = pixels[i * 4 + 2];
            rgb[i * 3 + 1] = pixels[i * 4 + 1];
            rgb[i * 3 + 2] = pixels[i * 4 + 0];
        }
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
