#pragma once

#include <string>

enum class ScoreCategory
{
    Local,
    Online,
    Country
};

struct ScoreEntry
{
    int place = 0;
    std::string name;
    int combo = 0;
    int score = 0;
};
