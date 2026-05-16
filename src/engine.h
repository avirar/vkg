#pragma once

#include "vkg.h"
#include <GLFW/glfw3.h>

class Engine {
public:
    Engine(GLFWwindow* window, bool wantHdr = false);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool beginFrame();
    void endFrame();
    void waitIdle();

    VkDevice device() const { return m_device; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkQueue computeQueue() const { return m_computeQueue; }
    VkCommandPool commandPool() const { return m_commandPool; }
    VkRenderPass renderPass() const { return m_renderPass; }
    VkExtent2D extent() const { return m_swapChainExtent; }
    VkFormat swapChainFormat() const { return m_swapChainFormat; }
    uint32_t currentImage() const { return m_currentImage; }
    VkFramebuffer currentFramebuffer() const { return m_swapChainFramebuffers[m_currentImage]; }
    VkFence currentFence() const { return m_inFlightFences[m_currentFrame]; }
    VkSemaphore imageAvailableSemaphore() const { return m_imageAvailableSemaphores[m_currentFrame]; }
    VkSemaphore renderFinishedSemaphore() const { return m_renderFinishedSemaphores[m_currentFrame]; }
    uint32_t currentFrame() const { return m_currentFrame; }
    VkImage currentSwapchainImage() const { return m_swapChainImages[m_currentImage]; }
    void setFramebufferResized() { m_framebufferResized = true; }
    bool hdrEnabled() const { return m_hdrEnabled; }
    void setHdrMaxLuminance(float lum) { m_hdrMaxLuminance = lum; }
    void toggleHdr();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmd);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer& buffer,
                      VkDeviceMemory& memory);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain(bool wantHdr);
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createSyncObjects();
    void recreateSwapChain();
    void cleanupSwapChain();
    void setHdrMetadata();

    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainFormat{};
    VkExtent2D m_swapChainExtent{};
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_currentFrame = 0;
    uint32_t m_currentImage = 0;
    bool m_framebufferResized = false;
    bool m_hdrEnabled = false;
    VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    float m_hdrMaxLuminance = 1000.0f;

    QueueFamilyIndices m_queueFamilies;
    SwapChainSupport m_swapChainSupport;
};
