#pragma once

class GraphicsConfig
{
public:
    bool vsync = true;
    int quality = 2;
    float gamma = 2.2f;
    int laneContrast = 2;
    int highwayGlow = 1;
    int hitFeedback = 2;

    void clamp();
};
