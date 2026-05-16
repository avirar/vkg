#pragma once

#include <string>
#include <unordered_map>

struct Config {
    int particles = 1000;
    bool fullscreen = false;
    int targetFps = 0;
    float pointScale = 1.0f;
    bool autoPointScale = true;

    std::string hypercolorVelocityMode = "color"; // "off", "color", "brightness"
    std::string hypercolorDistanceMode = "brightness"; // "off", "color", "brightness"
    float hyperVelocityIntensity = 8.0f;
    float hyperDistanceIntensity = 4.0f;
    float hyperVelLoR = 1.0f, hyperVelLoG = 0.19f, hyperVelLoB = 0.065f;
    float hyperVelHiR = 0.6f, hyperVelHiG = 0.8f, hyperVelHiB = 1.0f;
    float hyperDistLoR = 1.0f, hyperDistLoG = 0.19f, hyperDistLoB = 0.065f;
    float hyperDistHiR = 0.6f, hyperDistHiG = 0.8f, hyperDistHiB = 1.0f;

    bool hdr = true;
    float hdrMaxLuminance = 1000.0f;

    int singularityCount = 1;

    static Config load();
};

Config loadConfig(const std::string& path);
