#include "engine.h"
#include <set>
#include <algorithm>
#include <iostream>

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool ENABLE_VALIDATION = false;
#else
const bool ENABLE_VALIDATION = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*) {
    std::cerr << "[Vulkan] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) func(instance, debugMessenger, pAllocator);
}

// Utility functions
std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open " + filename);
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule sm;
    if (vkCreateShaderModule(device, &ci, nullptr, &sm) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");
    return sm;
}

// Engine implementation
Engine::Engine(GLFWwindow* window, bool wantHdr) : m_window(window) {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain(wantHdr);
    createRenderPass();
    createFramebuffers();
    createCommandPool();
    createSyncObjects();
}

Engine::~Engine() {
    vkDeviceWaitIdle(m_device);
    cleanupSwapChain();
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);
    if (m_debugMessenger)
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void Engine::createInstance() {
    bool validationAvailable = false;
    if (ENABLE_VALIDATION) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> available(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available.data());
        for (const auto& l : available)
            if (strcmp(l.layerName, VALIDATION_LAYERS[0]) == 0) validationAvailable = true;
        if (!validationAvailable)
            std::cerr << "Validation layers not available, running without\n";
    }

    VkApplicationInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName = "GL Gravitation Vulkan";
    ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.pEngineName = "vkg";
    ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ai.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &ai;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    // Check for swapchain_colorspace (needed for HDR)
    bool hasSwapchainColorSpace = false;
    {
        uint32_t extCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> available(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, available.data());
        for (const auto& e : available)
            if (strcmp(e.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
                hasSwapchainColorSpace = true;
    }
    if (hasSwapchainColorSpace) extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

    if (validationAvailable) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    ci.enabledExtensionCount = (uint32_t)extensions.size();
    ci.ppEnabledExtensionNames = extensions.data();

    if (validationAvailable) {
        ci.enabledLayerCount = (uint32_t)VALIDATION_LAYERS.size();
        ci.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }

    VkResult res = vkCreateInstance(&ci, nullptr, &m_instance);
    if (res != VK_SUCCESS) {
        std::string err = "Failed to create Vulkan instance: ";
        if (res == VK_ERROR_EXTENSION_NOT_PRESENT) err += "extension not present";
        else if (res == VK_ERROR_LAYER_NOT_PRESENT) err += "layer not present";
        else err += "code " + std::to_string(res);
        throw std::runtime_error(err);
    }

    // Create debug messenger
    if (validationAvailable) {
        VkDebugUtilsMessengerCreateInfoEXT dci{};
        dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dci.pfnUserCallback = debugCallback;
        CreateDebugUtilsMessengerEXT(m_instance, &dci, nullptr, &m_debugMessenger);
    }
}

void Engine::createSurface() {
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");
}

void Engine::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    auto rateDevice = [&](VkPhysicalDevice device) -> int {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures feats;
        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &feats);

        // Find queue families
        uint32_t qfCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &qfCount, nullptr);
        std::vector<VkQueueFamilyProperties> qfProps(qfCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &qfCount, qfProps.data());

        QueueFamilyIndices indices;
        for (uint32_t i = 0; i < qfCount; i++) {
            if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphics = i;
            if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) indices.compute = i;
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present);
            if (present) indices.present = i;
        }
        if (!indices.complete()) return 0;

        // Check swapchain support
        uint32_t fmtCount, modeCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &fmtCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &modeCount, nullptr);
        if (fmtCount == 0 || modeCount == 0) return 0;

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
        score += props.limits.maxImageDimension2D;
        m_queueFamilies = indices;
        return score;
    };

    int bestScore = 0;
    for (auto& d : devices) {
        int s = rateDevice(d);
        if (s > bestScore) { bestScore = s; m_physicalDevice = d; }
    }
    if (!m_physicalDevice) throw std::runtime_error("No suitable GPU found");
}

void Engine::createLogicalDevice() {
    // Recompute queue families
    uint32_t qfCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &qfCount, nullptr);
    std::vector<VkQueueFamilyProperties> qfProps(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &qfCount, qfProps.data());

    m_queueFamilies = {};
    for (uint32_t i = 0; i < qfCount; i++) {
        if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_queueFamilies.graphics = i;
        if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) m_queueFamilies.compute = i;
        VkBool32 present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &present);
        if (present) m_queueFamilies.present = i;
    }

    std::set<uint32_t> unique = {
        m_queueFamilies.graphics.value(),
        m_queueFamilies.compute.value(),
        m_queueFamilies.present.value()
    };
    std::vector<VkDeviceQueueCreateInfo> qcis;
    float prio = 1.0f;
    for (uint32_t qf : unique) {
        VkDeviceQueueCreateInfo qci{};
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = qf;
        qci.queueCount = 1;
        qci.pQueuePriorities = &prio;
        qcis.push_back(qci);
    }

    VkPhysicalDeviceFeatures feats{};
    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = (uint32_t)qcis.size();
    dci.pQueueCreateInfos = qcis.data();
    dci.pEnabledFeatures = &feats;

    std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // Check for HDR metadata extension
    {
        uint32_t extCount;
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> available(extCount);
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, available.data());
        for (const auto& e : available)
            if (strcmp(e.extensionName, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_EXT_HDR_METADATA_EXTENSION_NAME);
                break;
            }
    }

    dci.enabledExtensionCount = (uint32_t)extensions.size();
    dci.ppEnabledExtensionNames = extensions.data();

    if (vkCreateDevice(m_physicalDevice, &dci, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device");

    vkGetDeviceQueue(m_device, m_queueFamilies.graphics.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_queueFamilies.compute.value(), 0, &m_computeQueue);
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto& m : modes)
        if (m == VK_PRESENT_MODE_FIFO_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {
        std::clamp((uint32_t)w, caps.minImageExtent.width, caps.maxImageExtent.width),
        std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}

void Engine::createSwapChain(bool wantHdr) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_swapChainSupport.capabilities);

    uint32_t fc;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &fc, nullptr);
    m_swapChainSupport.formats.resize(fc);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &fc, m_swapChainSupport.formats.data());

    uint32_t pc;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &pc, nullptr);
    m_swapChainSupport.presentModes.resize(pc);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &pc, m_swapChainSupport.presentModes.data());

    VkSurfaceFormatKHR chosenFormat{};
    m_hdrEnabled = false;

    if (wantHdr) {
        // Try scRGB first (easiest — linear float, driver handles PQ conversion)
        for (const auto& f : m_swapChainSupport.formats) {
            if (f.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                f.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
                chosenFormat = f;
                m_hdrEnabled = true;
                break;
            }
        }
        // Fallback: try HDR10 PQ
        if (!m_hdrEnabled) {
            for (const auto& f : m_swapChainSupport.formats) {
                if (f.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                    f.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
                    chosenFormat = f;
                    m_hdrEnabled = true;
                    break;
                }
            }
        }
    }

    if (!m_hdrEnabled) {
        // SDR fallback: prefer sRGB over UNORM for auto-gamma
        for (const auto& f : m_swapChainSupport.formats)
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                chosenFormat = f;
                break;
            }
        if (chosenFormat.format == VK_FORMAT_UNDEFINED)
            chosenFormat = m_swapChainSupport.formats[0];
    }

    m_colorSpace = chosenFormat.colorSpace;
    auto mode = choosePresentMode(m_swapChainSupport.presentModes);
    auto extent = chooseExtent(m_swapChainSupport.capabilities, m_window);

    uint32_t imageCount = m_swapChainSupport.capabilities.minImageCount + 1;
    if (m_swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > m_swapChainSupport.capabilities.maxImageCount)
        imageCount = m_swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = m_surface;
    sci.minImageCount = imageCount;
    sci.imageFormat = chosenFormat.format;
    sci.imageColorSpace = chosenFormat.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    uint32_t qfIndices[] = { m_queueFamilies.graphics.value(), m_queueFamilies.present.value() };
    if (m_queueFamilies.graphics != m_queueFamilies.present) {
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = qfIndices;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    sci.preTransform = m_swapChainSupport.capabilities.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = mode;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &sci, nullptr, &m_swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain");

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainFormat = chosenFormat.format;
    m_swapChainExtent = extent;

    // Image views
    m_swapChainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo vci{};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = m_swapChainImages[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = m_swapChainFormat;
        vci.components = {VK_COMPONENT_SWIZZLE_IDENTITY};
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device, &vci, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");
    }

    // Set HDR metadata if enabled
    if (m_hdrEnabled)
        setHdrMetadata();
}

void Engine::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount = 1;
    rpi.pAttachments = &colorAttachment;
    rpi.subpassCount = 1;
    rpi.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_device, &rpi, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass");
}

void Engine::createFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = m_renderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = &m_swapChainImageViews[i];
        fci.width = m_swapChainExtent.width;
        fci.height = m_swapChainExtent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(m_device, &fci, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer");
    }
}

void Engine::createCommandPool() {
    VkCommandPoolCreateInfo cpi{};
    cpi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpi.queueFamilyIndex = m_queueFamilies.graphics.value();
    cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(m_device, &cpi, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
}

void Engine::createSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(m_device, &si, nullptr, &m_imageAvailableSemaphores[i]);
        vkCreateSemaphore(m_device, &si, nullptr, &m_renderFinishedSemaphores[i]);
        vkCreateFence(m_device, &fi, nullptr, &m_inFlightFences[i]);
    }
}

void Engine::recreateSwapChain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    while (w == 0 || h == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(m_window, &w, &h);
    }
    vkDeviceWaitIdle(m_device);
    cleanupSwapChain();
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to recreate window surface");
    createSwapChain(m_hdrEnabled);
    createRenderPass();
    createFramebuffers();
}

void Engine::cleanupSwapChain() {
    for (auto& fb : m_swapChainFramebuffers) vkDestroyFramebuffer(m_device, fb, nullptr);
    for (auto& iv : m_swapChainImageViews) vkDestroyImageView(m_device, iv, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

void Engine::setHdrMetadata() {
    if (!m_hdrEnabled) return;

    auto pfnSetHdrMetadata = (PFN_vkSetHdrMetadataEXT)
        vkGetDeviceProcAddr(m_device, "vkSetHdrMetadataEXT");
    if (!pfnSetHdrMetadata) return;

    VkHdrMetadataEXT hdrMetadata{};
    hdrMetadata.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;

    // BT.2020 primaries
    hdrMetadata.displayPrimaryRed.x   = 0.708f;
    hdrMetadata.displayPrimaryRed.y   = 0.292f;
    hdrMetadata.displayPrimaryGreen.x = 0.170f;
    hdrMetadata.displayPrimaryGreen.y = 0.797f;
    hdrMetadata.displayPrimaryBlue.x  = 0.131f;
    hdrMetadata.displayPrimaryBlue.y  = 0.046f;
    hdrMetadata.whitePoint.x          = 0.3127f;
    hdrMetadata.whitePoint.y          = 0.3290f;

    hdrMetadata.maxLuminance = m_hdrMaxLuminance;
    hdrMetadata.minLuminance = 0.0f;
    hdrMetadata.maxContentLightLevel = m_hdrMaxLuminance;
    hdrMetadata.maxFrameAverageLightLevel = m_hdrMaxLuminance * 0.5f;

    pfnSetHdrMetadata(m_device, 1, &m_swapChain, &hdrMetadata);
}

void Engine::toggleHdr() {
    m_hdrEnabled = !m_hdrEnabled;
    vkDeviceWaitIdle(m_device);
    cleanupSwapChain();
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to recreate window surface");
    createSwapChain(m_hdrEnabled);
    createRenderPass();
    createFramebuffers();
    if (m_hdrEnabled) setHdrMetadata();
}

bool Engine::beginFrame() {
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_currentImage);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swapchain image");

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    return true;
}

void Engine::endFrame() {
    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_swapChain;
    pi.pImageIndices = &m_currentImage;

    VkResult result = vkQueuePresentKHR(m_graphicsQueue, &pi);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::waitIdle() { vkDeviceWaitIdle(m_device); }

VkCommandBuffer Engine::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = m_commandPool;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &ai, &cmd);
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void Engine::endSingleTimeCommands(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(m_graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

void Engine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(m_device, &bi, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device, buffer, &memReq);

    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReq.size;
    ai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &ai, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    vkBindBufferMemory(m_device, buffer, memory, 0);
}

uint32_t Engine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    throw std::runtime_error("No suitable memory type found");
}
