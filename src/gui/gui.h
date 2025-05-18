#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <string>
#include <atomic>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

enum class AppState {
    SPLASH_SCREEN,
    MAIN_GAME
};

class GUI {
public:
    GUI();
    ~GUI(); // Use destructor for cleanup

    // Delete copy/move operations
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
    GUI(GUI&&) = delete;
    GUI& operator=(GUI&&) = delete;

    bool init(int width, int height, const std::string& title);
    void run(); // Main loop
    void requestShutdown(); // Signal the loop to exit

private:
    void shutdown(); // Internal cleanup function
    bool initSDL(int width, int height, const std::string& title);
    bool initBgfx(int width, int height);
    bool handleEvents(); // Process SDL events

    // Splash screen
    void updateSplashScreen(float deltaTime);
    void renderSplashScreen();

    // Main game
    void updateMainGame(float deltaTime);
    void renderMainGame();

    SDL_Window* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    std::atomic<bool> m_quitRequested{false}; // Flag to control main loop

    bgfx::Init m_bgfxInit;
    bool m_bgfxInitialized = false;

    // App state
    AppState m_currentState = AppState::SPLASH_SCREEN;

    // Splash screen timings
    float m_splashTimer = 0.0f;
    const float m_splashFadeInDuration = 1.5f;
    const float m_splashHoldDuration = 2.0f;
    const float m_splashFadeOutDuration = 1.5f;
    const float m_splashTotalDuration = m_splashFadeInDuration + m_splashHoldDuration + m_splashFadeOutDuration;

    // Main loop
    int64_t m_lastTime = 0;
};

#endif