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
            else if (key == "hypercolor_mode") cfg.hypercolorVelocityMode = val;
            else if (key == "hypercolor_velocity_mode") cfg.hypercolorVelocityMode = val;
            else if (key == "hypercolor_distance_mode") cfg.hypercolorDistanceMode = val;
            else if (key == "hypercolor") cfg.hypercolorVelocityMode = (val == "true" || val == "1" || val == "yes") ? "color" : "off";
            else if (key == "hyper_intensity") cfg.hyperVelocityIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_velocity_intensity") cfg.hyperVelocityIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_distance_intensity") cfg.hyperDistanceIntensity = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_lo_r") cfg.hyperVelLoR = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_lo_g") cfg.hyperVelLoG = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_lo_b") cfg.hyperVelLoB = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_hi_r") cfg.hyperVelHiR = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_hi_g") cfg.hyperVelHiG = (float)std::atof(val.c_str());
            else if (key == "hyper_vel_hi_b") cfg.hyperVelHiB = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_lo_r") cfg.hyperDistLoR = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_lo_g") cfg.hyperDistLoG = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_lo_b") cfg.hyperDistLoB = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_hi_r") cfg.hyperDistHiR = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_hi_g") cfg.hyperDistHiG = (float)std::atof(val.c_str());
            else if (key == "hyper_dist_hi_b") cfg.hyperDistHiB = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_r") cfg.hyperVelLoR = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_g") cfg.hyperVelLoG = (float)std::atof(val.c_str());
            else if (key == "hyper_lo_b") cfg.hyperVelLoB = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_r") cfg.hyperVelHiR = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_g") cfg.hyperVelHiG = (float)std::atof(val.c_str());
            else if (key == "hyper_hi_b") cfg.hyperVelHiB = (float)std::atof(val.c_str());
            else if (key == "hdr") cfg.hdr = !(val == "false" || val == "0" || val == "no");
            else if (key == "hdr_max_luminance") cfg.hdrMaxLuminance = (float)std::atof(val.c_str());
            else if (key == "singularity_count") cfg.singularityCount = std::atoi(val.c_str());
            else if (key == "osd") cfg.osdEnabled = (val == "true" || val == "1" || val == "yes");
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
            || cfg.hypercolorVelocityMode != "color" || cfg.hypercolorDistanceMode != "brightness"
            || cfg.hyperVelocityIntensity != 8.0f || cfg.hyperDistanceIntensity != 4.0f
            || cfg.hyperVelLoR != 1.0f || cfg.hyperVelLoG != 0.19f || cfg.hyperVelLoB != 0.065f
            || cfg.hyperVelHiR != 0.6f || cfg.hyperVelHiG != 0.8f || cfg.hyperVelHiB != 1.0f
            || cfg.hyperDistLoR != 1.0f || cfg.hyperDistLoG != 0.19f || cfg.hyperDistLoB != 0.065f
            || cfg.hyperDistHiR != 0.6f || cfg.hyperDistHiG != 0.8f || cfg.hyperDistHiB != 1.0f
            || !cfg.hdr || cfg.hdrMaxLuminance != 1000.0f || cfg.singularityCount != 1)
            return cfg;
    }
    return Config{};
}
