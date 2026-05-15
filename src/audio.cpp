#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"
#include <iostream>

Audio::Audio() {
    ma_result result = ma_engine_init(nullptr, &m_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        m_initialized = false;
        return;
    }
    m_initialized = true;
}

Audio::~Audio() {
    if (m_loaded) {
        ma_sound_uninit(&m_sound);
    }
    if (m_initialized) {
        ma_engine_uninit(&m_engine);
    }
}

bool Audio::load(const char* filename) {
    if (!m_initialized) return false;

    ma_result result = ma_sound_init_from_file(&m_engine, filename, 0, nullptr, nullptr, &m_sound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load audio file: " << filename << std::endl;
        return false;
    }
    m_loaded = true;
    return true;
}

void Audio::play() {
    if (!m_loaded) return;

    ma_sound_stop(&m_sound);
    ma_sound_seek_to_pcm_frame(&m_sound, 0);
    ma_sound_start(&m_sound);
}
