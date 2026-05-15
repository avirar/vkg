#include "simulation.h"
#include <cmath>
#include <algorithm>

Simulation::Simulation(uint32_t initialParticles) : m_rng(42) {
    m_state.particleCount = initialParticles;
}

void Simulation::update(float dt) {
    if (dt <= 0.0f) return;
    m_justReinitialized = false;

    updateRotation(dt);
    updateSingularity(dt);
    updateCountdown();
    adjustParticleCount(dt);

    m_state.accumulatedTime += dt;
    m_state.physicsFrozen = (m_state.reinitCountdown > 0);
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

void Simulation::updateRotation(float dt) {
    // glg.c:2271: rotation_angle += dt * 10.0
    m_state.rotationAngle += dt * 10.0f;
    if (m_state.rotationAngle >= 360.0f) m_state.rotationAngle -= 360.0f;
    if (m_state.rotationAngle < 0.0f) m_state.rotationAngle += 360.0f;
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

void Simulation::updateCountdown() {
    m_state.reinitCountdown--;
    if (m_state.reinitCountdown <= 0) {
        reinitialize();
    }
}

void Simulation::adjustParticleCount(float dt) {
    if (dt <= 0.0f) return;
    if (m_state.physicsFrozen) return;

    // glg.c:2633-2646: sqrt(target_speed / actual_speed) * current_count
    float ratio = std::sqrt(TARGET_SPEED / dt);
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

void Simulation::reinitialize() {
    m_justReinitialized = true;

    // Reset countdown
    m_state.reinitCountdown = REINIT_COUNTDOWN_START;

    // Reset singularity (glg.c:2649-2660)
    m_state.singularityX = 0.0f;
    m_state.singularityY = 0.0f;
    m_state.singularityZ = 0.0f;
    m_state.singularityVX = 0.0f;
    m_state.singularityVY = 0.0f;
    m_state.singularityVZ = 0.0f;

    // Reset accumulated time (glg.c:2700)
    m_state.accumulatedTime = 0.0f;

    // Adjust particle count based on performance
    // Note: the compute shader handles the spherical redistribution
}

void Simulation::forceReinit() {
    m_state.reinitCountdown = 0;
}
