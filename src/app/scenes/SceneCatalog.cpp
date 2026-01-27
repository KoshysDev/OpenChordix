#include "SceneCatalog.h"

#include <array>
#include <cctype>

namespace
{
    std::string toLower(std::string_view text)
    {
        std::string out(text);
        for (char &ch : out)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return out;
    }

    constexpr std::array<std::pair<std::string_view, GraphicsFlow::SceneId>, 11> kSceneAliases{{
        {"intro", GraphicsFlow::SceneId::Intro},
        {"audio", GraphicsFlow::SceneId::AudioSetup},
        {"audio-setup", GraphicsFlow::SceneId::AudioSetup},
        {"menu", GraphicsFlow::SceneId::MainMenu},
        {"main", GraphicsFlow::SceneId::MainMenu},
        {"main-menu", GraphicsFlow::SceneId::MainMenu},
        {"track", GraphicsFlow::SceneId::TrackSelect},
        {"track-select", GraphicsFlow::SceneId::TrackSelect},
        {"tuner", GraphicsFlow::SceneId::Tuner},
        {"settings", GraphicsFlow::SceneId::Settings},
        {"test", GraphicsFlow::SceneId::Test},
    }};

    constexpr std::array<std::string_view, 7> kSceneNames{{
        "intro",
        "audio",
        "menu",
        "track",
        "tuner",
        "settings",
        "test",
    }};
}

std::optional<GraphicsFlow::SceneId> SceneCatalog::fromName(std::string_view name)
{
    const std::string id = toLower(name);
    for (const auto &[alias, sceneId] : kSceneAliases)
    {
        if (alias == id)
        {
            return sceneId;
        }
    }
    return std::nullopt;
}

std::vector<std::string> SceneCatalog::names()
{
    std::vector<std::string> names;
    names.reserve(kSceneNames.size());
    for (auto name : kSceneNames)
    {
        names.emplace_back(name);
    }
    return names;
}
