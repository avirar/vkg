#pragma once

#include <random>

struct SimState {
    float orbitAngle = 0.0f;
    float wobblePhase = 0.0f;
    float singularityX = 0.0f, singularityY = 0.0f, singularityZ = 0.0f;
    float singularityVX = 0.0f, singularityVY = 0.0f, singularityVZ = 0.0f;
    float accumulatedTime = 0.0f;
    uint32_t particleCount = 1000;
    float aspectRatioX = 1.0f;
    float aspectRatioY = 1.0f;
};

class Simulation {
public:
    Simulation(uint32_t initialParticles = 1000);

    void update(float dt);
    void resize(uint32_t width, uint32_t height);
    void setTargetFps(int fps);

    const SimState& state() const { return m_state; }

private:
    void updateCamera(float dt);
    void updateSingularity(float dt);
    void adjustParticleCount(float dt);

    SimState m_state;
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
    static constexpr uint32_t MAX_PARTICLES = 32768;
};
