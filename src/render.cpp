#include "render.h"
#include <array>
#include <iostream>
#include <cmath>

Renderer::Renderer(Engine& engine) : m_engine(engine) {
    createCommandBuffers();
    createSunVertexBuffer();
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
    ias.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

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

    VkVertexInputAttributeDescription attrDescs[2]{};
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[0].offset = offsetof(Particle, screen_x);

    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32_SFLOAT;
    attrDescs[1].offset = offsetof(Particle, brightness);

    VkPipelineVertexInputStateCreateInfo vis{};
    vis.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount = 1;
    vis.pVertexBindingDescriptions = &bindDesc;
    vis.vertexAttributeDescriptionCount = 2;
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

    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = setLayouts;

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

void Renderer::drawFrame(float sinRot, float cosRot,
                          float singX, float singY, float singZ,
                          float aspectX, float aspectY) {
    uint32_t frameIdx = m_engine.currentFrame();
    VkCommandBuffer cmd = m_commandBuffers[frameIdx];

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &bi);

    // Dispatch compute shader
    if (m_compute) m_compute->dispatch(cmd);

    // Begin render pass
    VkRenderPassBeginInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = m_engine.renderPass();
    rpi.framebuffer = m_engine.currentFramebuffer();
    rpi.renderArea.extent = m_engine.extent();
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.05f, 1.0f}}};
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
        // Project singularity to screen
        float camDist = 1.5f;
        float camOff = 0.0f;
        float rotZ = cosRot * singZ + sinRot * singX;
        float persp = camDist / (rotZ + camOff);
        float sunScrX = (cosRot * singX - sinRot * singZ) * persp * aspectX;
        float sunScrY = singY * persp * aspectY;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sunPipeline);

        if (m_textures) {
            VkDescriptorSet sunSet = m_textures->sunDescriptorSet();
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_sunPipelineLayout, 0, 1, &sunSet, 0, nullptr);
        }

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_sunVertexBuffer, &offset);

        // 7 layers matching original (glg.c:2377-2485): 0.20, 0.08, 0.04, 0.02, 0.02, 0.01, 0.01
        float baseSize = 0.20f * aspectX; // size relative to aspect ratio
        SunPushConstants spc{};
        spc.centerX = sunScrX;
        spc.centerY = sunScrY;

        // Layer 1: 0.20
        spc.scale = baseSize;
        spc.alpha = 0.25f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 2: 0.08
        spc.scale = baseSize * 0.4f;
        spc.alpha = 0.35f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 3: 0.04
        spc.scale *= 0.5f;
        spc.alpha = 0.45f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 4: 0.02
        spc.scale *= 0.5f;
        spc.alpha = 0.55f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 5: same size
        spc.alpha = 0.65f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 6: 0.01
        spc.scale *= 0.5f;
        spc.alpha = 0.80f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Layer 7: same size
        spc.alpha = 1.0f;
        vkCmdPushConstants(cmd, m_sunPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(spc), &spc);
        vkCmdDraw(cmd, 4, 1, 0, 0);
    }

    // --- Draw particles ---
    if (m_compute && m_compute->particleCount() > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

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
}
