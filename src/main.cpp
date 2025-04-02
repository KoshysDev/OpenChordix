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

// --- Basic AudioManager Class ---
class AudioManager {
    public:
        // Constructor: Takes desired API, initializes RtAudio
        explicit AudioManager(RtAudio::Api api = RtAudio::Api::UNSPECIFIED)
          : api_(api) // Store the requested API
        {
            std::cout << "Attempting to initialize RtAudio with API: "
                      << RtAudio::getApiDisplayName(api_) << " (" << api_ << ")" << std::endl;
    
            // Use std::unique_ptr for automatic resource management
            // Pass our error callback to the RtAudio constructor
            try {
                audio_ = std::make_unique<RtAudio>(api_, errorCallback);
                // Check if the desired API was actually used (optional check)
                if (api_ != RtAudio::Api::UNSPECIFIED && audio_->getCurrentApi() != api_) {
                     std::cerr << "Warning: Requested API (" << RtAudio::getApiDisplayName(api_)
                               << ") was not available or chosen. Using API: "
                               << RtAudio::getApiDisplayName(audio_->getCurrentApi()) << std::endl;
                     api_ = audio_->getCurrentApi(); // Update the stored API to the actual one
                }
                 std::cout << "RtAudio initialized successfully using API: "
                           << RtAudio::getApiDisplayName(audio_->getCurrentApi()) << std::endl;
    
            } catch (const std::exception& e) {
                // Catch potential exceptions during RtAudio object creation itself
                // (Though RtAudio primarily uses callbacks/return codes for operational errors)
                errorCallback(RTAUDIO_SYSTEM_ERROR, "Exception during RtAudio instantiation: " + std::string(e.what()));
                 // Re-throw or handle appropriately; here we re-throw to signal construction failure
                 throw std::runtime_error("Failed to initialize RtAudio instance.");
            }
    
            // Basic check after construction
            if (audio_->getDeviceCount() == 0 && api_ != RtAudio::Api::RTAUDIO_DUMMY) {
                 // The error callback should have already printed something,
                 // but we can add an extra warning here.
                 std::cerr << "Warning: RtAudio initialized, but no devices were found for the selected API.\n";
            }
        }
    
        // Destructor - unique_ptr handles RtAudio cleanup automatically
    
        // --- Device Listing Method ---
        bool listDevices() const {
            if (!audio_) {
                 errorCallback(RTAUDIO_INVALID_USE, "listDevices called on uninitialized AudioManager.");
                return false;
            }
    
            unsigned int deviceCount = audio_->getDeviceCount();
            std::cout << "\nFound " << deviceCount << " audio devices for API "
                      << RtAudio::getApiDisplayName(audio_->getCurrentApi()) << ":" << std::endl;
    
            if (deviceCount < 1) {
                std::cerr << "No audio devices found for this API.\n";
                return false; // Indicate no devices found
            }
    
            std::vector<unsigned int> deviceIds = audio_->getDeviceIds();
            bool devices_listed = false;
    
            for (unsigned int id : deviceIds) {
                try {
                    RtAudio::DeviceInfo info = audio_->getDeviceInfo(id);
    
                    std::cout << "  Device ID " << id << ": " << info.name;
                    if (info.isDefaultInput) std::cout << " (DEFAULT INPUT)";
                    if (info.isDefaultOutput) std::cout << " (DEFAULT OUTPUT)";
                    std::cout << std::endl;
                    std::cout << "    Output Channels: " << info.outputChannels << std::endl;
                    std::cout << "    Input Channels: " << info.inputChannels << std::endl;
                    // Add other details as needed (sample rates, etc.)
                     std::cout << "    Sample Rates: ";
                     if (info.sampleRates.empty()) {
                         std::cout << "(None reported)";
                     } else {
                         for (unsigned int rate : info.sampleRates) {
                             std::cout << rate << " ";
                         }
                     }
                     std::cout << std::endl;
                     devices_listed = true; // We successfully listed at least one device
    
                } catch (const std::exception& e) {
                     std::cerr << "Standard Exception getting info for device " << id << ": " << e.what() << std::endl;
                     // Continue to the next device
                }
            }
            std::cout << std::endl;
    
            // Print default device IDs reported by the current API context
            std::cout << "Default Output Device ID (for this API): " << audio_->getDefaultOutputDevice() << std::endl;
            std::cout << "Default Input Device ID (for this API): " << audio_->getDefaultInputDevice() << std::endl;
    
            return devices_listed; // Return true if we successfully listed info for at least one device
        }
    
        // --- Static method to list compiled APIs ---
        static void listAvailableApis() {
            std::vector<RtAudio::Api> apis;
            RtAudio::getCompiledApi(apis);
    
            std::cout << "\nCompiled RtAudio APIs:" << std::endl;
            if (apis.empty()) {
                std::cout << "  (No specific APIs compiled? This is unusual, should include Dummy)" << std::endl;
            } else {
                for (const auto& api_enum : apis) {
                    std::cout << "  - " << RtAudio::getApiDisplayName(api_enum)
                              << " (" << api_enum << ")" << std::endl;
                }
            }
             std::cout << std::endl;
        }
    
         // --- Getter for the underlying RtAudio object (use with caution) ---
         // Often better to expose specific functionality via AudioManager methods
         // RtAudio* getRtAudioInstance() const { return audio_.get(); }
    
         // --- Getter for the currently used API ---
          RtAudio::Api getCurrentApi() const {
              return audio_ ? audio_->getCurrentApi() : RtAudio::Api::UNSPECIFIED;
          }
    
         // --- Getter for default input device ID ---
         unsigned int getDefaultInputDeviceId() const {
             return audio_ ? audio_->getDefaultInputDevice() : 0; // 0 is invalid ID
         }
    
    
    private:
        std::unique_ptr<RtAudio> audio_;
        RtAudio::Api api_; // Store the API we attempted to initialize with or the one actually used
    };
    
    
    //-----------------------------------------------------------------------------
    // Main Function
    //-----------------------------------------------------------------------------
    int main() {
        std::cout << "OpenChordix - RtAudio Initializer" << std::endl;
        std::cout << "RtAudio Version: " << RtAudio::getVersion() << std::endl;
    
        // 1. List available APIs first, so the user knows the options
        AudioManager::listAvailableApis();
    
        // 2. Choose the API to use
        // ********************************************************************
        // ****** CHANGE THIS VALUE to select a different API if desired ******
        // ********************************************************************
        // Options based on your previous output might include:
        // RtAudio::Api::LINUX_ALSA
        // RtAudio::Api::LINUX_PULSE
        // RtAudio::Api::UNIX_JACK (if compiled & JACK server running)
        // RtAudio::Api::UNSPECIFIED (let RtAudio choose)
        RtAudio::Api selectedApi = RtAudio::Api::LINUX_ALSA; // <<< TRY ALSA FIRST
        // RtAudio::Api selectedApi = RtAudio::Api::LINUX_PULSE; // <<< OR TRY PULSE
        // RtAudio::Api selectedApi = RtAudio::Api::UNSPECIFIED; // <<< OR LET RTAUDIO CHOOSE
    
        try {
            // 3. Create the AudioManager instance with the selected API
            AudioManager manager(selectedApi);
    
            // 4. List the devices found by the initialized API
            manager.listDevices();
    
            // --- Ready for next steps ---
            std::cout << "\nAudioManager initialized with API: "
                      << RtAudio::getApiDisplayName(manager.getCurrentApi()) << std::endl;
            std::cout << "Default input device ID from manager: " << manager.getDefaultInputDeviceId() << std::endl;
            // Next: Add methods to AudioManager to open/start/stop streams...
    
        } catch (const std::runtime_error& e) {
             std::cerr << "Fatal Error: Could not initialize AudioManager. " << e.what() << std::endl;
             return 1;
        } catch (const std::exception& e) {
             std::cerr << "Fatal Error: An unexpected exception occurred. " << e.what() << std::endl;
             return 1;
        }
    
    
        std::cout << "\nProgram finished successfully." << std::endl;
        return 0;
    }
}