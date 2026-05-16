#pragma once

#include <string>
#include <unordered_map>

struct Config {
    int particles = 1000;
    bool fullscreen = false;
    int targetFps = 0;
    float pointScale = 1.0f;

    std::string hypercolorMode = "color"; // "off", "color", "brightness"
    float hyperIntensity = 8.0f;
    float hyperLoR = 1.0f, hyperLoG = 0.19f, hyperLoB = 0.065f;
    float hyperHiR = 0.6f, hyperHiG = 0.8f, hyperHiB = 1.0f;

    bool hdr = true;
    float hdrMaxLuminance = 1000.0f;

    static Config load();
};

Config loadConfig(const std::string& path);
