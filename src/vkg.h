#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>
#include <cstring>
#include <fstream>
#include <stdexcept>

constexpr int MAX_FRAMES_IN_FLIGHT = 3;
constexpr int MAX_PARTICLES = 32768;
constexpr int REINIT_COUNTDOWN_START = 2048;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> present;

    bool complete() const {
        return graphics.has_value() && compute.has_value() && present.has_value();
    }
};

struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Particle {
    float pos_x, pos_y, pos_z;
    float vel_x, vel_y, vel_z;
    float screen_x, screen_y;
    float brightness;
    float _pad; // align to 16 bytes
};
static_assert(sizeof(Particle) == 40, "Particle must be 40 bytes");

std::vector<char> readFile(const std::string& filename);

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
