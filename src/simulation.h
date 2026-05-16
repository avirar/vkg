#pragma once

#include <random>
#include <array>

struct Singularity {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float pulsePhase = 0.0f;
};

struct SimState {
    float orbitAngle = 0.0f;
    float wobblePhase = 0.0f;
    float sunPulse = 1.0f;
    std::array<Singularity, 8> singularities;
    uint32_t singularityCount = 1;
    float comX = 0.0f, comY = 0.0f, comZ = 0.0f;
    float accumulatedTime = 0.0f;
    uint32_t particleCount = 1000;
    float aspectRatioX = 1.0f;
    float aspectRatioY = 1.0f;
};

class Simulation {
public:
    Simulation(uint32_t initialParticles = 1000, uint32_t singCount = 1);

    void update(float dt);
    void resize(uint32_t width, uint32_t height);
    void setTargetFps(int fps);

    const SimState& state() const { return m_state; }
    uint32_t maxParticles() const { return m_state.particleCount; }

private:
    void updateCamera(float dt);
    void updateSingularities(float dt);
    void updateCenterOfMass();
    void adjustParticleCount(float dt);

    SimState m_state;
    uint32_t m_maxCount = 0;
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist{0, 1000};
    float m_targetSpeed = 0.01f;

    static constexpr float GRAVITY = 0.01f;
    static constexpr float DAMPING = 0.982f;
    static constexpr float SINGULARITY_VEL_RANGE = 0.08f;
    static constexpr float SINGULARITY_POS_RANGE = 0.2f;
    static constexpr float CAMERA_DIST = 1.5f;
    static constexpr float CAMERA_OFFSET = 0.6f;
    static constexpr uint32_t MIN_PARTICLES = 2;
};
