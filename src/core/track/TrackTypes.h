#pragma once

#include <string>
#include <vector>

struct TrackPart
{
    std::string name;
};

struct TrackInfo
{
    std::string title;
    std::string artist;
    std::string source;
    std::string mapper;
    int bpm = 0;
    std::string length;
    std::vector<TrackPart> parts;
};
