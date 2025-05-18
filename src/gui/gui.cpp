#include "gui.h"
#include <iostream>
#include <stdexcept>

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

GUI::GUI() = default;

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

    // Loop until explicitly told to quit by events or requestShutdown()
    while (!m_quitRequested.load()) {

        // 1. Handle Input and Window Events
        if (!handleEvents()) {
            m_quitRequested.store(true); // SDL_QUIT or Escape key triggered exit
            continue;
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