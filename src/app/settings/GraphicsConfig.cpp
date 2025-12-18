#include "settings/GraphicsConfig.h"

#include <algorithm>

void GraphicsConfig::clamp()
{
    quality = std::clamp(quality, 0, 3);
    gamma = std::clamp(gamma, 1.4f, 2.6f);
    laneContrast = std::clamp(laneContrast, 0, 3);
    highwayGlow = std::clamp(highwayGlow, 0, 3);
    hitFeedback = std::clamp(hitFeedback, 0, 3);
}
