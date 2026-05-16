#pragma once

#include <string>
#include <unordered_map>

struct Config {
    int particles = 1000;
    bool fullscreen = false;
    int targetFps = 0;
    float pointScale = 1.0f;

    static Config load();
};

Config loadConfig(const std::string& path);
