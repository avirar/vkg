#pragma once

#include "miniaudio.h"

class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool load(const char* filename);
    void play();

private:
    ma_engine m_engine{};
    ma_sound m_sound{};
    bool m_initialized = false;
    bool m_loaded = false;
};
