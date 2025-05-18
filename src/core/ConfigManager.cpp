#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <SDL2/SDL_filesystem.h>
#include <filesystem>

ConfigManager::ConfigManager(const std::string& configFileName) {
    char* prefPath_c = SDL_GetPrefPath("WrongWireStudio", "OpenChordix");
    if (prefPath_c) {
        m_configFilePath = std::string(prefPath_c) + configFileName;
        SDL_free(prefPath_c);
    } else {
        // Fallback if SDL_GetPrefPath fails
        m_configFilePath = configFileName;
        std::cerr << "Warning: Could not get preferential path for settings. Using local file: "
                  << m_configFilePath << std::endl;
    }
    std::cout << "Config file path: " << m_configFilePath << std::endl;
    loadConfig(); // Try to load on construction
}

bool ConfigManager::loadConfig() {
    std::ifstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Config file not found or could not be opened: " << m_configFilePath << ". Using defaults." << std::endl;
        m_audioConfig = AudioConfig(); // Reset to defaults
        m_audioConfig.configured = false;
        return false;
    }

    try {
        json j;
        configFile >> j;
        m_audioConfig = j.get<AudioConfig>();
        std::cout << "Config loaded successfully." << std::endl;
        if (!m_audioConfig.configured) {
             std::cout << "Config loaded but marked as not configured." << std::endl;
        }
        return true;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << ". Using defaults." << std::endl;
        m_audioConfig = AudioConfig(); // Reset to defaults on parse error
        m_audioConfig.configured = false;
        return false;
    }
}

bool ConfigManager::saveConfig() const {
    std::ofstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open config file for saving: " << m_configFilePath << std::endl;
        return false;
    }

    try {
        json j = m_audioConfig;
        configFile << std::setw(4) << j << std::endl; // Pretty print
        std::cout << "Config saved successfully to " << m_configFilePath << std::endl;
        return true;
    } catch (const json::exception& e) {
        std::cerr << "Error serializing config to JSON: " << e.what() << std::endl;
        return false;
    }
}

AudioConfig& ConfigManager::getAudioConfig() {
    return m_audioConfig;
}

const AudioConfig& ConfigManager::getAudioConfig() const {
    return m_audioConfig;
}

bool ConfigManager::isFirstLaunch() const {
    // Considered first launch if file doesn't exist OR if it exists but `configured` is false.
    std::ifstream configFile(m_configFilePath);
    if (!configFile.is_open()) {
        return true; // File doesn't exist
    }
    // If file exists, rely on the loaded m_audioConfig.configured status
    return !m_audioConfig.configured;
}