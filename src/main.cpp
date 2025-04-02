#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <rtaudio/RtAudio.h>

//-----------------------------------------------------------------------------
// Error Callback Function (Matches RtAudioErrorCallback signature)
// This function will be called by RtAudio when an error or warning occurs.
//-----------------------------------------------------------------------------
void errorCallback( RtAudioErrorType type, const std::string &errorText )
{
    // RtAudioErrorType::RTAUDIO_WARNING is for non-critical issues
    if ( type == RTAUDIO_WARNING ) {
        std::cerr << "RtAudio Warning: " << errorText << std::endl;
    } else if ( type != RTAUDIO_NO_ERROR) {
        // Treat other non-zero types as errors
        std::cerr << "RtAudio Error: " << errorText << std::endl;
    }
}

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
int main() {
    std::cout << "OpenChordix - RtAudio Device Lister" << std::endl;

    // --- Instantiate RtAudio ---
    // Pass error callback function to the constructor.
    // let RtAudio choose the API
    RtAudio audio(RtAudio::Api::UNSPECIFIED, errorCallback);

    // --- Check for Devices ---
    unsigned int deviceCount = audio.getDeviceCount();
    if ( deviceCount < 1 ) {
        std::cerr << "\nNo audio devices found!\n";
        return 1;
    }

    std::cout << "\nFound " << deviceCount << " audio devices:" << std::endl;

    // List Devices
    std::vector<unsigned int> deviceIds = audio.getDeviceIds();

    for ( unsigned int id : deviceIds ) {
        try {
            // Use RtAudio::DeviceInfo as it's a nested struct
            RtAudio::DeviceInfo info = audio.getDeviceInfo(id);

            std::cout << "  Device ID " << id << ": " << info.name;
            if (info.isDefaultInput) std::cout << " (DEFAULT INPUT)";
            if (info.isDefaultOutput) std::cout << " (DEFAULT OUTPUT)";
            std::cout << std::endl;
            std::cout << "    Output Channels: " << info.outputChannels << std::endl;
            std::cout << "    Input Channels: " << info.inputChannels << std::endl;
            std::cout << "    Duplex Channels: " << info.duplexChannels << std::endl;
            std::cout << "    Sample Rates: ";
            if (info.sampleRates.empty()) {
                std::cout << "(None reported)";
            } else {
                for (unsigned int rate : info.sampleRates) {
                    std::cout << rate << " ";
                }
            }
            std::cout << std::endl;
            std::cout << "    Preferred Sample Rate: " << info.preferredSampleRate << std::endl;

        } catch (const std::exception& e) {
             std::cerr << "Standard Exception getting info for device " << id << ": " << e.what() << std::endl;
        }
    }
    std::cout << std::endl;

    // --- Display Default Devices ---
    unsigned int defaultOut = audio.getDefaultOutputDevice();
    unsigned int defaultIn = audio.getDefaultInputDevice();
    std::cout << "Default Output Device ID: " << defaultOut << std::endl;
    std::cout << "Default Input Device ID: " << defaultIn << std::endl;


    std::cout << "\nDevice listing complete." << std::endl;

    return 0;
}