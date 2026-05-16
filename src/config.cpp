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
            else if (key == "point_scale") cfg.pointScale = (float)std::atof(val.c_str());
            else if (key == "auto_point_scale") cfg.autoPointScale = !(val == "false" || val == "0" || val == "no");
            else if (key == "hypercolor_mode") cfg.hypercolorMode = val;
            else if (key == "hypercolor") cfg.hypercolorMode = (val == "true" || val == "1" || val == "yes") ? "color" : "off";
            else if (key == "hyper_intensity") cfg.hyperVelocityIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_velocity_intensity") cfg.hyperVelocityIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_distance_intensity") cfg.hyperDistanceIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_r") cfg.hyperLoR = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_g") cfg.hyperLoG = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_b") cfg.hyperLoB = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_r") cfg.hyperHiR = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_g") cfg.hyperHiG = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_b") cfg.hyperHiB = (float)std::atof(val.c_str());
            else if (key == "hdr") cfg.hdr = !(val == "false" || val == "0" || val == "no");
            else if (key == "hdr_max_luminance") cfg.hdrMaxLuminance = (float)std::atof(val.c_str());
            else if (key == "singularity_count") cfg.singularityCount = std::atoi(val.c_str());
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
        if (cfg.particles != 1000 || cfg.fullscreen || cfg.targetFps != 0
            || cfg.pointScale != 1.0f || !cfg.autoPointScale
            || cfg.hypercolorMode != "color" || cfg.hyperVelocityIntensity != 8.0f || cfg.hyperDistanceIntensity != 0.0f
            || cfg.hyperLoR != 1.0f || cfg.hyperLoG != 0.19f || cfg.hyperLoB != 0.065f
            || cfg.hyperHiR != 0.6f || cfg.hyperHiG != 0.8f || cfg.hyperHiB != 1.0f
            || !cfg.hdr || cfg.hdrMaxLuminance != 1000.0f || cfg.singularityCount != 1)
            return cfg;
    }
    return Config{};
}
