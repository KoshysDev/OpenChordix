#pragma once

#include <vector>
#include <string>

#include "Scene.h"
#include "audio/AudioSession.h"
#include "AnimatedUI.h"

class TunerScene : public Scene
{
public:
    TunerScene(AudioSession &audio, AnimatedUI &ui);

    void render(float dt, const FrameInput &input, GraphicsContext &gfx, std::atomic<bool> &quitFlag) override;
    bool finished() const override { return finished_; }

private:
    struct StringTarget
    {
        int midi = -1;
        float frequency = 0.0f;
        std::string label;
    };

    struct TuningProfile
    {
        std::string name;
        std::string subtitle;
        std::vector<StringTarget> strings;
    };

    void drawTuningSelector();
    void drawStringSelector();
    void drawLivePanel(const PitchState &pitch, const StringTarget &target, int stringIndex);
    int activeStringIndex(const PitchState &pitch);
    int detectStringFromPitch(const PitchState &pitch) const;

    static StringTarget makeString(int midi);
    static float midiToFrequency(int midi);
    static std::string midiToLabel(int midi);

    AudioSession &audio_;
    AnimatedUI &ui_;
    std::vector<TuningProfile> tunings_;
    size_t tuningIndex_ = 0;
    int selectedString_ = 0;
    int lastAutoString_ = 0;
    bool autoDetectString_ = true;
    bool finished_ = false;
};
