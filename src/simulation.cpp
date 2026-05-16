#include "simulation.h"
#include <cmath>
#include <algorithm>

Simulation::Simulation(uint32_t initialParticles) : m_rng(42) {
    m_state.particleCount = initialParticles;
}

void Simulation::update(float dt) {
    if (dt <= 0.0f) return;

    updateCamera(dt);
    updateSingularity(dt);
    adjustParticleCount(dt);

    m_state.accumulatedTime += dt;
}

void Simulation::setTargetFps(int fps) {
    if (fps > 0)
        m_targetSpeed = 1.0f / (float)fps;
}

void Simulation::resize(uint32_t width, uint32_t height) {
    // Aspect ratio: larger dimension = 1.0, smaller = ratio
    if (width >= height) {
        m_state.aspectRatioX = (float)height / (float)width;
        m_state.aspectRatioY = 1.0f;
    } else {
        m_state.aspectRatioX = 1.0f;
        m_state.aspectRatioY = (float)width / (float)height;
    }
}

void Simulation::updateCamera(float dt) {
    float degPerSec = 4.0f;

    m_state.orbitAngle += dt * degPerSec;
    if (m_state.orbitAngle >= 360.0f) m_state.orbitAngle -= 360.0f;
    if (m_state.orbitAngle < 0.0f) m_state.orbitAngle += 360.0f;

    m_state.wobblePhase += dt;
}

void Simulation::updateSingularity(float dt) {
    // Random walk velocity (glg.c:2300-2305)
    m_state.singularityVX += (500.0f - (float)m_dist(m_rng)) * 2e-05f;
    m_state.singularityVY += (500.0f - (float)m_dist(m_rng)) * 2e-05f;
    m_state.singularityVZ += (500.0f - (float)m_dist(m_rng)) * 2e-05f;

    // Clamp velocities (glg.c:2306-2326)
    m_state.singularityVX = std::clamp(m_state.singularityVX,
        -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);
    m_state.singularityVY = std::clamp(m_state.singularityVY,
        -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);
    m_state.singularityVZ = std::clamp(m_state.singularityVZ,
        -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);

    // Update position (always active — matches observed original behavior)
    m_state.singularityX += m_state.singularityVX * dt;
    m_state.singularityY += m_state.singularityVY * dt;
    m_state.singularityZ += m_state.singularityVZ * dt;

    // Bounce off boundaries (glg.c:2332-2352)
    if (std::abs(m_state.singularityX) > SINGULARITY_POS_RANGE) {
        m_state.singularityVX = -m_state.singularityVX;
        m_state.singularityX += m_state.singularityVX * dt;
    }
    if (std::abs(m_state.singularityY) > SINGULARITY_POS_RANGE) {
        m_state.singularityVY = -m_state.singularityVY;
        m_state.singularityY += m_state.singularityVY * dt;
    }
    if (std::abs(m_state.singularityZ) > SINGULARITY_POS_RANGE) {
        m_state.singularityVZ = -m_state.singularityVZ;
        m_state.singularityZ += m_state.singularityVZ * dt;
    }
}

void Simulation::adjustParticleCount(float dt) {
    if (dt <= 0.0f) return;

    // glg.c:2633-2646: sqrt(target_speed / actual_speed) * current_count
    float ratio = std::sqrt(m_targetSpeed / dt);
    float newCount = ratio * (float)m_state.particleCount;

    if (newCount <= (float)MAX_PARTICLES) {
        if (newCount >= (float)MIN_PARTICLES) {
            m_state.particleCount = (uint32_t)std::round(newCount);
        } else {
            m_state.particleCount = MIN_PARTICLES;
        }
    } else {
        m_state.particleCount = MAX_PARTICLES;
    }
}
