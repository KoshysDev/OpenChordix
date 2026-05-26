#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <rtaudio/RtAudio.h>

struct AudioManagerTestState
{
    bool throwOnConstruct = false;
    bool listDevicesResult = true;
    bool openMonitoringStreamResult = true;
    bool startStreamResult = true;
    bool stopStreamResult = true;
    bool streamRunning = true;
    float latestPitchHz = 0.0f;
    unsigned int defaultInputDeviceId = 0;
    unsigned int defaultOutputDeviceId = 0;
    unsigned int openedInputDeviceId = 0;
    unsigned int openedOutputDeviceId = 0;
    unsigned int openedSampleRate = 0;
    unsigned int openedBufferFrames = 0;
    bool stopCalled = false;
    bool closeCalled = false;
    std::vector<RtAudio::Api> availableApis{};
    std::vector<RtAudio::Api> constructedApis{};
    std::unordered_map<unsigned int, RtAudio::DeviceInfo> deviceInfos{};
};

class AudioManager
{
public:
    explicit AudioManager(RtAudio::Api api = RtAudio::Api::UNSPECIFIED)
        : api_(api)
    {
        auto &testState = state();
        testState.constructedApis.push_back(api);
        if (testState.throwOnConstruct)
        {
            throw std::runtime_error("stub constructor failure");
        }
    }

    static AudioManagerTestState &state()
    {
        static AudioManagerTestState testState;
        return testState;
    }

    static void resetTestState()
    {
        state() = AudioManagerTestState{};
    }

    static std::vector<RtAudio::Api> getAvailableApis()
    {
        return state().availableApis;
    }

    bool listDevices() const
    {
        return state().listDevicesResult;
    }

    std::optional<RtAudio::DeviceInfo> getDeviceInfo(unsigned int deviceId) const
    {
        const auto &testState = state();
        auto it = testState.deviceInfos.find(deviceId);
        if (it == testState.deviceInfos.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    bool openMonitoringStream(unsigned int inputDeviceId,
                              unsigned int outputDeviceId,
                              unsigned int sampleRate = 44100,
                              unsigned int bufferFrames = 256)
    {
        auto &testState = state();
        testState.openedInputDeviceId = inputDeviceId;
        testState.openedOutputDeviceId = outputDeviceId;
        testState.openedSampleRate = sampleRate;
        testState.openedBufferFrames = bufferFrames;
        return testState.openMonitoringStreamResult;
    }

    bool startStream()
    {
        return state().startStreamResult;
    }

    bool stopStream()
    {
        auto &testState = state();
        testState.stopCalled = true;
        return testState.stopStreamResult;
    }

    void closeStream()
    {
        state().closeCalled = true;
    }

    bool isStreamRunning() const
    {
        return state().streamRunning;
    }

    RtAudio::Api getCurrentApi() const
    {
        return api_;
    }

    unsigned int getDefaultInputDeviceId() const
    {
        return state().defaultInputDeviceId;
    }

    unsigned int getDefaultOutputDeviceId() const
    {
        return state().defaultOutputDeviceId;
    }

    float getLatestPitchHz() const
    {
        return state().latestPitchHz;
    }

private:
    RtAudio::Api api_;
};
