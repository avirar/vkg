#pragma once

#include "engine.h"

class Compute {
public:
    Compute(Engine& engine);
    ~Compute();

    void init();
    void update(float dt);

private:
    Engine& m_engine;
};
