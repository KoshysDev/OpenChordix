set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BX_DIR ${BGFX_ROOT}/bx)
set(BIMG_DIR ${BGFX_ROOT}/bimg)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

add_library(openchordix_renderer STATIC
    Renderer.cpp
    assets/GltfLoader.cpp
    ${BGFX_DIR}/examples/common/imgui/imgui.cpp
    ${BGFX_DIR}/3rdparty/dear-imgui/imgui.cpp
    ${BGFX_DIR}/3rdparty/dear-imgui/imgui_draw.cpp
    ${BGFX_DIR}/3rdparty/dear-imgui/imgui_widgets.cpp
    ${BGFX_DIR}/3rdparty/dear-imgui/imgui_tables.cpp
    ${BGFX_DIR}/3rdparty/dear-imgui/imgui_demo.cpp
)

target_include_directories(openchordix_renderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Headers for bgfx stack
target_include_directories(openchordix_renderer PRIVATE
    ${BGFX_DIR}/include
    ${BX_DIR}/include
    ${BIMG_DIR}/include
    ${BIMG_DIR}/3rdparty
    ${BGFX_DIR}/3rdparty
    ${BGFX_DIR}/examples/common
)

target_compile_definitions(openchordix_renderer PRIVATE
    USE_ENTRY=0
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:BX_CONFIG_DEBUG=0>
)

find_package(fastgltf CONFIG REQUIRED)

if(WIN32)
    find_package(glfw3 CONFIG REQUIRED)
    set(OPENCHORDIX_GLFW_LIB glfw)
else()
    find_package(glfw3 CONFIG QUIET)
    if(TARGET glfw)
        set(OPENCHORDIX_GLFW_LIB glfw)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GLFW3 REQUIRED glfw3)
        target_include_directories(openchordix_renderer PUBLIC ${GLFW3_INCLUDE_DIRS})
        set(OPENCHORDIX_GLFW_LIB ${GLFW3_LIBRARIES})
    endif()
endif()

if(OPENCHORDIX_ENABLE_WAYLAND)
    target_compile_definitions(openchordix_renderer PRIVATE OPENCHORDIX_ENABLE_WAYLAND=1)
endif()

# Where bgfx's makefile puts libs
set(BGFX_LIB_DIR ${BGFX_DIR}/.build/linux64_gcc/bin)

# Decide which config to use
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BGFX_LIBS
        ${BGFX_LIB_DIR}/libbgfxDebug.a
        ${BGFX_LIB_DIR}/libbimgDebug.a
        ${BGFX_LIB_DIR}/libbxDebug.a
        ${BGFX_LIB_DIR}/libbimg_decodeDebug.a
    )
else()
    set(BGFX_LIBS
        ${BGFX_LIB_DIR}/libbgfxRelease.a
        ${BGFX_LIB_DIR}/libbimgRelease.a
        ${BGFX_LIB_DIR}/libbxRelease.a
        ${BGFX_LIB_DIR}/libbimg_decodeRelease.a
    )
endif()

# Link bgfx stack + platform libs
if(WIN32)
    target_link_libraries(openchordix_renderer PRIVATE
        ${BGFX_LIBS}
        fastgltf::fastgltf
        ${OPENCHORDIX_GLFW_LIB}
    )
else()
    target_link_libraries(openchordix_renderer PRIVATE
        ${BGFX_LIBS}
        fastgltf::fastgltf
        ${OPENCHORDIX_GLFW_LIB}
        GL
        X11
        dl
        pthread
    )
endif()
