#include "config.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <string>

static std::string trim(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) i++;
    while (j > i && (s[j-1] == ' ' || s[j-1] == '\t' || s[j-1] == '\r')) j--;
    return s.substr(i, j - i);
}

static int safeAtoi(const std::string& s, int def) {
    try { return std::stoi(s); } catch (...) { return def; }
}

static float safeAtof(const std::string& s, float def) {
    try { return std::stof(s); } catch (...) { return def; }
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
            if (key == "particles") cfg.particles = safeAtoi(val, cfg.particles);
            else if (key == "fullscreen") cfg.fullscreen = (val == "true" || val == "1" || val == "yes");
            else if (key == "target_fps") cfg.targetFps = safeAtoi(val, cfg.targetFps);
            else if (key == "point_scale") cfg.pointScale = safeAtof(val, cfg.pointScale);
            else if (key == "auto_point_scale") cfg.autoPointScale = !(val == "false" || val == "0" || val == "no");
            else if (key == "hypercolor_mode") cfg.hypercolorVelocityMode = val;
            else if (key == "hypercolor_velocity_mode") cfg.hypercolorVelocityMode = val;
            else if (key == "hypercolor_distance_mode") cfg.hypercolorDistanceMode = val;
            else if (key == "hypercolor") cfg.hypercolorVelocityMode = (val == "true" || val == "1" || val == "yes") ? "color" : "off";
            else if (key == "hyper_intensity") cfg.hyperVelocityIntensity = safeAtof(val, cfg.hyperVelocityIntensity);
            else if (key == "hyper_velocity_intensity") cfg.hyperVelocityIntensity = safeAtof(val, cfg.hyperVelocityIntensity);
            else if (key == "hyper_distance_intensity") cfg.hyperDistanceIntensity = safeAtof(val, cfg.hyperDistanceIntensity);
            else if (key == "hyper_vel_lo_r") cfg.hyperVelLoR = safeAtof(val, cfg.hyperVelLoR);
            else if (key == "hyper_vel_lo_g") cfg.hyperVelLoG = safeAtof(val, cfg.hyperVelLoG);
            else if (key == "hyper_vel_lo_b") cfg.hyperVelLoB = safeAtof(val, cfg.hyperVelLoB);
            else if (key == "hyper_vel_hi_r") cfg.hyperVelHiR = safeAtof(val, cfg.hyperVelHiR);
            else if (key == "hyper_vel_hi_g") cfg.hyperVelHiG = safeAtof(val, cfg.hyperVelHiG);
            else if (key == "hyper_vel_hi_b") cfg.hyperVelHiB = safeAtof(val, cfg.hyperVelHiB);
            else if (key == "hyper_dist_lo_r") cfg.hyperDistLoR = safeAtof(val, cfg.hyperDistLoR);
            else if (key == "hyper_dist_lo_g") cfg.hyperDistLoG = safeAtof(val, cfg.hyperDistLoG);
            else if (key == "hyper_dist_lo_b") cfg.hyperDistLoB = safeAtof(val, cfg.hyperDistLoB);
            else if (key == "hyper_dist_hi_r") cfg.hyperDistHiR = safeAtof(val, cfg.hyperDistHiR);
            else if (key == "hyper_dist_hi_g") cfg.hyperDistHiG = safeAtof(val, cfg.hyperDistHiG);
            else if (key == "hyper_dist_hi_b") cfg.hyperDistHiB = safeAtof(val, cfg.hyperDistHiB);
            else if (key == "hyper_lo_r") cfg.hyperVelLoR = safeAtof(val, cfg.hyperVelLoR);
            else if (key == "hyper_lo_g") cfg.hyperVelLoG = safeAtof(val, cfg.hyperVelLoG);
            else if (key == "hyper_lo_b") cfg.hyperVelLoB = safeAtof(val, cfg.hyperVelLoB);
            else if (key == "hyper_hi_r") cfg.hyperVelHiR = safeAtof(val, cfg.hyperVelHiR);
            else if (key == "hyper_hi_g") cfg.hyperVelHiG = safeAtof(val, cfg.hyperVelHiG);
            else if (key == "hyper_hi_b") cfg.hyperVelHiB = safeAtof(val, cfg.hyperVelHiB);
            else if (key == "hdr") cfg.hdr = !(val == "false" || val == "0" || val == "no");
            else if (key == "hdr_max_luminance") cfg.hdrMaxLuminance = safeAtof(val, cfg.hdrMaxLuminance);
            else if (key == "singularity_count") cfg.singularityCount = safeAtoi(val, cfg.singularityCount);
            else if (key == "blend_alpha_scale") cfg.blendAlphaScale = safeAtof(val, cfg.blendAlphaScale);
            else if (key == "color_cap") cfg.colorCap = safeAtof(val, cfg.colorCap);
            else if (key == "osd") cfg.osdEnabled = (val == "true" || val == "1" || val == "yes");
        }
    }
    return cfg;
}

Config Config::load() {
    // Search: cwd for vkg.ini or vkg.conf
    const char* candidates[] = { "vkg.ini", "vkg.conf" };
    for (const auto& c : candidates) {
        std::ifstream test(c);
        if (test.is_open()) {
            test.close();
            return loadConfig(c);
        }
    }
    return Config{};
}
