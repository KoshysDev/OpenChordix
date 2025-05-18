#include "gui.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <bx/math.h>
#include <bx/timer.h>

// Platform-specific window handles
#include <SDL2/SDL_syswm.h>

// Platform specific headers for bgfx::PlatformData
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    #include <X11/Xlib.h>
#elif BX_PLATFORM_OSX
    // #include <objc/runtime.h>
#elif BX_PLATFORM_WINDOWS
    #include <windows.h>
#endif

// --- Constructor / Destructor ---

GUI::GUI() {
    m_lastTime = bx::getHPCounter();
}

GUI::~GUI() {
    shutdown();
}

// --- Public Methods ---

bool GUI::init(int width, int height, const std::string& title) {
    m_width = width;
    m_height = height;

    if (!initSDL(width, height, title)) {
        return false;
    }

    if (!initBgfx(width, height)) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        m_window = nullptr;
        return false;
    }

    return true;
}

void GUI::requestShutdown() {
    m_quitRequested.store(true);
}

// --- Private Helper Methods ---

bool GUI::initSDL(int width, int height, const std::string& title) {
    std::cout << "Initializing SDL..." << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // m_window = SDL_CreateWindow(
    //     title.c_str(),
    //     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    //     width, height,
    //     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE // Allow resizing
    // );

    Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0,
        window_flags
    );

    if (m_window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Save size for bgfx
    SDL_GetWindowSize(m_window, &m_width, &m_height);

    std::cout << "SDL Initialized and Window created in fullscreen-desktop mode with size: " << m_width << "x" << m_height << "." << std::endl;

    return true;
}

bool GUI::initBgfx(int width, int height) {
    std::cout << "Initializing BGFX..." << std::endl;
    bgfx::PlatformData pd;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(m_window, &wmi)) {
        std::cerr << "Failed to get native window info! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Setup bgfx::PlatformData based on the platform SDL is running on
    #if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
        pd.ndt = wmi.info.x11.display;
        pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
    #elif BX_PLATFORM_OSX
        pd.ndt = nullptr;
        pd.nwh = wmi.info.cocoa.window;
    #elif BX_PLATFORM_WINDOWS
        pd.ndt = nullptr;
        pd.nwh = wmi.info.win.window;
    #else
        #error "Platform not supported yet."
    #endif
    pd.context = nullptr;
    pd.backBuffer = nullptr;
    pd.backBufferDS = nullptr;

    bgfx::setPlatformData(pd);

    // Initialize bgfx
    bgfx::Init bgfxInit;
    // Automatically choose the best renderer type (Direct3D, OpenGL, Metal, Vulkan)
    bgfxInit.type = bgfx::RendererType::Count; // Auto-select
    bgfxInit.vendorId = BGFX_PCI_ID_NONE; // Auto-select
    bgfxInit.platformData = pd; // Use the platform data
    bgfxInit.resolution.width = (uint32_t)width;
    bgfxInit.resolution.height = (uint32_t)height;
    bgfxInit.resolution.reset = BGFX_RESET_VSYNC; // Start with vsync

    std::cout << "Calling bgfx::init..." << std::endl;
    if (!bgfx::init(bgfxInit)) {
        std::cerr << "Failed to initialize BGFX!" << std::endl;
        m_bgfxInitialized = false;
        return false;
    }
    m_bgfxInit = bgfxInit;
    m_bgfxInitialized = true;

    std::cout << "BGFX Initialized. Renderer: " << bgfx::getRendererName(bgfx::getRendererType()) << std::endl;

    // Set view 0 clear state
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0); // Clear to dark grey
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height)); // Set view rect for view 0

    return true;
}

void GUI::updateSplashScreen(float deltaTime) {
    m_splashTimer += deltaTime;

    if(m_splashTimer >= m_splashTotalDuration) {
        m_currentState = AppState::MAIN_GAME;
        std::cout << "Splash screen finished." << std::endl;
        //clear view
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
    }
}

void GUI::renderSplashScreen() {
    float alpha = 0.0f;
    uint32_t baseColorRGB = 0x00000000;

    if (m_splashTimer < m_splashFadeInDuration) {
        alpha = m_splashTimer / m_splashFadeInDuration;
    }else if (m_splashTimer < m_splashFadeInDuration + m_splashHoldDuration) {
        alpha = 1.0f;
    } else if (m_splashTimer < m_splashTotalDuration) {
        float fadeOutProgress = (m_splashTimer - (m_splashFadeInDuration + m_splashHoldDuration)) / m_splashFadeOutDuration;
        alpha = 1.0f - fadeOutProgress;
    } else {
        alpha = 0.0f;
    }

    // Clamp alpha
    alpha = std::max(0.0f, std::min(1.0f, alpha));
    uint8_t alpha_u8 = static_cast<uint8_t>(alpha * 255.0f);

    // Lerp between colors
    uint32_t splashIntroColor = 0x1A1A1AFF;
    uint32_t mainGameColor = 0x1A1A1AFF;

    uint8_t sr = (splashIntroColor >> 24) & 0xFF;
    uint8_t sg = (splashIntroColor >> 16) & 0xFF;
    uint8_t sb = (splashIntroColor >> 8) & 0xFF;

    uint8_t mr = (mainGameColor >> 24) & 0xFF;
    uint8_t mg = (mainGameColor >> 16) & 0xFF;
    uint8_t mb = (mainGameColor >> 8) & 0xFF;

    float t = 0.0f; // Interpolation value

    if(m_splashTimer < m_splashFadeInDuration) {
        t = m_splashTimer / m_splashFadeInDuration;
    } else if (m_splashTimer < m_splashFadeInDuration + m_splashHoldDuration) {
        t = 1.0f;
    } else if (m_splashTimer < m_splashFadeInDuration) {
        t = 1.0f - ((m_splashTimer - (m_splashFadeInDuration + m_splashHoldDuration)) / m_splashFadeOutDuration); 
    } else {
        t = 0.0f;
    }

    t = std::max(0.0f, std::min(1.0f, t));

    uint8_t r = static_cast<uint8_t>(mr + (sr - mr) * t);
    uint8_t g = static_cast<uint8_t>(mg + (sg - mg) * t);
    uint8_t b = static_cast<uint8_t>(mb + (sb - mb) * t);
    uint32_t finalClearColor = (r << 24) | (g << 16) | (b << 8) | 0xFF;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, finalClearColor, 1.0f, 0);
}

void GUI::updateMainGame(float deltaTime) {
    (void)deltaTime;
}

void GUI::renderMainGame() {}

bool GUI::handleEvents() {
    SDL_Event event;
    bool running = true;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                running = false; // Signal loop exit
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                    std::cout << "Window resized to " << m_width << "x" << m_height << std::endl;
                    // Important: Reset bgfx's internal buffers
                    bgfx::reset((uint32_t)m_width, (uint32_t)m_height, m_bgfxInit.resolution.reset);
                    // Re-apply view settings for the new size
                    bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
                }
                break;
             // Handle for keyboard/mouse
             case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                     running = false; // Quit on Escape key
                }
                break;
        }
    }
    return running;
}

void GUI::run() {
    if (!m_window || !m_bgfxInitialized) { // Check if init succeeded
         std::cerr << "GUI not initialized properly, cannot run." << std::endl;
         return;
    }

    std::cout << "Starting main loop..." << std::endl;

    // Reset m_lastTime
    m_lastTime = bx::getHPCounter();

    // Loop until explicitly told to quit by events or requestShutdown()
    while (!m_quitRequested.load()) {

        // Calculate delta time
        int64_t now = bx::getHPCounter();
        int64_t frameTime = now - m_lastTime;
        m_lastTime = now;
        const int64_t freq = bx::getHPFrequency();
        float deltaTime = (float)frameTime / freq;

        // 1. Handle Input and Window Events
        if (!handleEvents()) {
            m_quitRequested.store(true); // SDL_QUIT or Escape key triggered exit
            continue;
        }

        // --- Update based on state ---
        if (m_currentState == AppState::SPLASH_SCREEN) {
            updateSplashScreen(deltaTime);
        } else if (m_currentState == AppState::MAIN_GAME) {
            updateMainGame(deltaTime);
        }

        // --- Rendering ---
        bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
        float viewMtx[16], projMtx[16];
        bx::mtxIdentity(viewMtx);
        bx::mtxOrtho(projMtx, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, viewMtx, projMtx);

        if (m_currentState == AppState::SPLASH_SCREEN) {
            renderSplashScreen();
        } else if (m_currentState == AppState::MAIN_GAME) {
            renderMainGame();
        }

        bgfx::touch(0);

        bgfx::frame();
    }

    std::cout << "Main loop finished." << std::endl;
}

void GUI::shutdown() {
    std::cout << "Shutting down GUI..." << std::endl;
    if (m_bgfxInitialized) { // Check if bgfx was initialized
        std::cout << "Shutting down BGFX..." << std::endl;
        bgfx::shutdown();
        m_bgfxInitialized = false;
    }

    if (m_window) {
        std::cout << "Destroying SDL Window..." << std::endl;
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    std::cout << "Quitting SDL..." << std::endl;
    SDL_Quit();
    std::cout << "GUI Shutdown complete." << std::endl;
}