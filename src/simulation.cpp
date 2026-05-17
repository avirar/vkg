#include "simulation.h"
#include <cmath>
#include <algorithm>

Simulation::Simulation(uint32_t initialParticles, uint32_t singCount) : m_rng(42) {
    m_state.particleCount = initialParticles;
    m_maxCount = initialParticles;
    m_state.singularityCount = std::min(singCount, 8u);
    if (m_state.singularityCount < 1) m_state.singularityCount = 1;

    // Initialize singularities at random positions
    std::uniform_real_distribution<float> posDist(-0.15f, 0.15f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 6.28318f);
    for (uint32_t i = 0; i < m_state.singularityCount; i++) {
        m_state.singularities[i].x = posDist(m_rng);
        m_state.singularities[i].y = posDist(m_rng);
        m_state.singularities[i].z = posDist(m_rng);
        m_state.singularities[i].vx = 0.0f;
        m_state.singularities[i].vy = 0.0f;
        m_state.singularities[i].vz = 0.0f;
        m_state.singularities[i].pulsePhase = phaseDist(m_rng);
    }
    updateCenterOfMass();
}

void Simulation::update(float dt) {
    if (dt <= 0.0f) return;

    updateCamera(dt);
    updateSingularities(dt);
    updateCenterOfMass();
    adjustParticleCount(dt);

    m_state.accumulatedTime += dt;
}

void Simulation::setTargetFps(int fps) {
    if (fps > 0)
        m_targetSpeed = 1.0f / (float)fps;
}

void Simulation::resize(uint32_t width, uint32_t height) {
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
    while (m_state.orbitAngle >= 360.0f) m_state.orbitAngle -= 360.0f;
    while (m_state.orbitAngle < 0.0f) m_state.orbitAngle += 360.0f;

    m_state.wobblePhase += dt;
    m_state.sunPulse = 1.0f + 0.25f * std::sin(m_state.wobblePhase * 0.5f);
}

void Simulation::updateSingularities(float dt) {
    for (uint32_t i = 0; i < m_state.singularityCount; i++) {
        auto& s = m_state.singularities[i];

        s.vx += (500.0f - (float)m_dist(m_rng)) * 2e-05f;
        s.vy += (500.0f - (float)m_dist(m_rng)) * 2e-05f;
        s.vz += (500.0f - (float)m_dist(m_rng)) * 2e-05f;

        s.vx = std::clamp(s.vx, -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);
        s.vy = std::clamp(s.vy, -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);
        s.vz = std::clamp(s.vz, -SINGULARITY_VEL_RANGE, SINGULARITY_VEL_RANGE);

        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.z += s.vz * dt;

        if (std::abs(s.x) > SINGULARITY_POS_RANGE) {
            s.vx = -s.vx;
            s.x += s.vx * dt;
        }
        if (std::abs(s.y) > SINGULARITY_POS_RANGE) {
            s.vy = -s.vy;
            s.y += s.vy * dt;
        }
        if (std::abs(s.z) > SINGULARITY_POS_RANGE) {
            s.vz = -s.vz;
            s.z += s.vz * dt;
        }
    }
}

void Simulation::updateCenterOfMass() {
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    for (uint32_t i = 0; i < m_state.singularityCount; i++) {
        cx += m_state.singularities[i].x;
        cy += m_state.singularities[i].y;
        cz += m_state.singularities[i].z;
    }
    float inv = 1.0f / (float)m_state.singularityCount;
    m_state.comX = cx * inv;
    m_state.comY = cy * inv;
    m_state.comZ = cz * inv;
}

void Simulation::adjustParticleCount(float dt) {
    if (dt <= 0.0f) return;

    // Dampened adjustment: exponential moving average with 0.3 smoothing factor
    float ratio = std::sqrt(m_targetSpeed / dt);
    float target = ratio * (float)m_state.particleCount;
    float smoothed = m_state.particleCount * 0.7f + target * 0.3f;

    if (smoothed <= (float)m_maxCount) {
        if (smoothed >= (float)MIN_PARTICLES) {
            m_state.particleCount = (uint32_t)std::round(std::max(0.0f, smoothed));
        } else {
            m_state.particleCount = MIN_PARTICLES;
        }
    } else {
        m_state.particleCount = m_maxCount;
    }
}
