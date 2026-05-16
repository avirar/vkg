#include "config.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

static std::string trim(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) i++;
    while (j > i && (s[j-1] == ' ' || s[j-1] == '\t' || s[j-1] == '\r')) j--;
    return s.substr(i, j - i);
}

Config loadConfig(const std::string& path) {
    Config cfg;
    std::ifstream f(path);
    if (!f.is_open()) return cfg;

    std::string line;
    std::string section;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos)
                section = line.substr(1, end - 1);
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (section == "general" || section.empty()) {
            if (key == "particles") cfg.particles = std::atoi(val.c_str());
            else if (key == "fullscreen") cfg.fullscreen = (val == "true" || val == "1" || val == "yes");
            else if (key == "target_fps") cfg.targetFps = std::atoi(val.c_str());
        }
    }
    return cfg;
}

Config Config::load() {
    // Search: cwd, executable dir, home
    std::string candidates[] = {
        "vkg.ini",
        "vkg.conf",
    };
    for (auto& c : candidates) {
        Config cfg = loadConfig(c);
        if (cfg.particles != 1000 || cfg.fullscreen || cfg.targetFps != 0)
            return cfg;
    }
    return Config{};
}
