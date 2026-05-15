#pragma once

#include <random>

struct SimState {
    float rotationAngle = 0.0f;
    float singularityX = 0.0f, singularityY = 0.0f, singularityZ = 0.0f;
    float singularityVX = 0.0f, singularityVY = 0.0f, singularityVZ = 0.0f;
    float accumulatedTime = 0.0f;
    int reinitCountdown = 2048;
    uint32_t particleCount = 1000;
    bool firstInit = true;
    float aspectRatioX = 1.0f;
    float aspectRatioY = 1.0f;
    bool physicsFrozen = false;
};

class Simulation {
public:
    Simulation(uint32_t initialParticles = 1000);

    void update(float dt);
    void resize(uint32_t width, uint32_t height);

    const SimState& state() const { return m_state; }
    bool justReinitialized() const { return m_justReinitialized; }
    void clearReinitFlag() { m_justReinitialized = false; }
    void forceReinit();

private:
    void updateRotation(float dt);
    void updateSingularity(float dt);
    void updateCountdown();
    void adjustParticleCount(float dt);
    void reinitialize();

    SimState m_state;
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist{0, 1000};
    bool m_justReinitialized = false;

    static constexpr float GRAVITY = 0.01f;
    static constexpr float DAMPING = 0.982f;
    static constexpr float SINGULARITY_VEL_RANGE = 0.08f;
    static constexpr float SINGULARITY_POS_RANGE = 0.2f;
    static constexpr float TARGET_SPEED = 0.01f;
    static constexpr float CAMERA_DIST = 1.5f;
    static constexpr float CAMERA_OFFSET = 0.0f;
    static constexpr int REINIT_COUNTDOWN_START = 2048;
    static constexpr uint32_t MIN_PARTICLES = 2;
    static constexpr uint32_t MAX_PARTICLES = 32768;
};
