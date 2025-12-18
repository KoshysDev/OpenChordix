#include "console/ConsolePrompter.h"

#include <iostream>
#include <limits>

#include "ui/DeviceSelector.h"

ConsolePrompter::ConsolePrompter(std::vector<RtAudio::Api> apis) : apis_(std::move(apis)) {}

RtAudio::Api ConsolePrompter::chooseApi() const
{
    std::cout << "\nPlease choose an audio API:" << std::endl;
    for (size_t i = 0; i < apis_.size(); ++i)
    {
        std::cout << "  " << (i + 1) << ". " << RtAudio::getApiDisplayName(apis_[i]) << " (" << apis_[i] << ")" << std::endl;
    }
    std::cout << "  " << (apis_.size() + 1) << ". Auto Select (UNSPECIFIED)" << std::endl;

    int apiChoice = 0;
    while (true)
    {
        std::cout << "Enter choice (1-" << (apis_.size() + 1) << "): ";
        std::cin >> apiChoice;
        if (std::cin.fail() || apiChoice < 1 || apiChoice > static_cast<int>(apis_.size() + 1))
        {
            std::cerr << "Invalid input..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
    }

    if (apiChoice <= static_cast<int>(apis_.size()))
    {
        return apis_[static_cast<size_t>(apiChoice - 1)];
    }
    return RtAudio::Api::UNSPECIFIED;
}

std::optional<unsigned int> ConsolePrompter::chooseDevice(AudioManager &manager, DeviceRole role, unsigned int defaultId) const
{
    const std::string roleName = role == DeviceRole::Input ? "INPUT" : "OUTPUT";
    while (true)
    {
        std::cout << "\nEnter the Device ID of the " << roleName << " device you want to use";
        if (defaultId != 0)
        {
            std::cout << " [" << defaultId << "]";
        }
        std::cout << ": ";

        std::string line;
        if (!std::getline(std::cin, line))
        {
            return std::nullopt;
        }

        unsigned int deviceId = defaultId;
        if (!line.empty())
        {
            try
            {
                deviceId = static_cast<unsigned int>(std::stoul(line));
            }
            catch (...)
            {
                std::cerr << "Invalid input. Please enter a number." << std::endl;
                continue;
            }
        }
        else if (defaultId == 0)
        {
            std::cerr << "Please enter a device ID." << std::endl;
            continue;
        }

        RtAudio::DeviceInfo info{};
        try
        {
            info = manager.getDeviceInfo(deviceId);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error validating device ID: " << e.what() << std::endl;
            return std::nullopt;
        }

        if ((info.name.empty() && info.inputChannels == 0 && info.outputChannels == 0) || channelCount(info, role) == 0)
        {
            std::cerr << "Device ID " << deviceId << " is not a valid " << roleName << " device." << std::endl;
            continue;
        }

        std::cout << "Selected " << roleName << " device: " << info.name << " (ID: " << deviceId << ")" << std::endl;
        return deviceId;
    }
}
