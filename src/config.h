#pragma once

#include <string>
#include <unordered_map>

struct Config {
    int particles = 1000;
    bool fullscreen = false;
    int targetFps = 0;
    float pointScale = 1.0f;

    bool hypercolor = true;
    float hyperIntensity = 8.0f;
    float hyperLoR = 1.0f, hyperLoG = 0.19f, hyperLoB = 0.065f;
    float hyperHiR = 0.6f, hyperHiG = 0.8f, hyperHiB = 1.0f;
    float particleColorR = 1.0f, particleColorG = 0.19f, particleColorB = 0.065f;

    static Config load();
};

Config loadConfig(const std::string& path);
