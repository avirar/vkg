#include "render.h"
#include <array>
#include <iostream>

Renderer::Renderer(Engine& engine) : m_engine(engine) {
    createCommandBuffers();
    createGraphicsPipeline();
}

Renderer::~Renderer() {
    vkDestroyPipeline(m_engine.device(), m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_engine.device(), m_graphicsPipelineLayout, nullptr);
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

void Renderer::createGraphicsPipeline() {
    // Shaders
    auto vertCode = readFile("shaders/quad.vert.spv");
    auto fragCode = readFile("shaders/quad.frag.spv");
    VkShaderModule vertModule = createShaderModule(m_engine.device(), vertCode);
    VkShaderModule fragModule = createShaderModule(m_engine.device(), fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // Vertex input: particle buffer fields (screen_x, screen_y at offset 24, brightness at offset 32)
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

    // Input assembly: point list
    VkPipelineInputAssemblyStateCreateInfo ias{};
    ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    ias.primitiveRestartEnable = VK_FALSE;

    // Viewport / scissor (dynamic)
    VkPipelineViewportStateCreateInfo vs{};
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs.viewportCount = 1;
    vs.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth/stencil: disabled (no depth test)
    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    // Color blend: additive (SRC_ALPHA, ONE, ADD) — matches glg.c:878-879
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

    // Dynamic state
    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dsc{};
    dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsc.dynamicStateCount = (uint32_t)dynamicStates.size();
    dsc.pDynamicStates = dynamicStates.data();

    // Pipeline layout (empty for now — no descriptor sets needed)
    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

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

    vkDestroyShaderModule(m_engine.device(), vertModule, nullptr);
    vkDestroyShaderModule(m_engine.device(), fragModule, nullptr);
}

void Renderer::drawFrame() {
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &bi);

    // Dispatch compute shader if available
    if (m_compute) {
        m_compute->dispatch(cmd);
    }

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

    // Draw particles
    if (m_compute && m_compute->particleCount() > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        VkViewport vp{};
        vp.x = 0; vp.y = 0;
        vp.width = (float)m_engine.extent().width;
        vp.height = (float)m_engine.extent().height;
        vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);

        VkRect2D scissor{};
        scissor.extent = m_engine.extent();
        vkCmdSetScissor(cmd, 0, 1, &scissor);

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
    if (vkQueueSubmit(m_engine.graphicsQueue(), 1, &si, fence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer");
}
