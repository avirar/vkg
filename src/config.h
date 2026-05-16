#pragma once

#include <string>
#include <unordered_map>

struct Config {
    int particles = 1000;
    bool fullscreen = false;
    int targetFps = 0; // 0 = auto from VSync, else manual target

    static Config load();
};

Config loadConfig(const std::string& path);
