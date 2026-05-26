#pragma once

struct ImVec2
{
    float x = 0.0f;
    float y = 0.0f;

    constexpr ImVec2() = default;
    constexpr ImVec2(float xValue, float yValue)
        : x(xValue),
          y(yValue)
    {
    }
};
