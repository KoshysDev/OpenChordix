add_library(openchordix_renderer STATIC
    Renderer.cpp
)

target_include_directories(openchordix_renderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Paths to bx/bimg/bgfx sources (relative to project root)
set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BX_DIR ${BGFX_ROOT}/bx)
set(BIMG_DIR ${BGFX_ROOT}/bimg)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

# Headers for bgfx stack
target_include_directories(openchordix_renderer PRIVATE
    ${BGFX_DIR}/include
    ${BX_DIR}/include
    ${BIMG_DIR}/include
    ${BGFX_DIR}/3rdparty
    ${BGFX_DIR}/examples/common
)

if(WIN32)
    find_package(glfw3 CONFIG REQUIRED)
    set(OPENCHORDIX_GLFW_LIB glfw)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED glfw3)
    target_include_directories(openchordix_renderer PUBLIC ${GLFW3_INCLUDE_DIRS})
    set(OPENCHORDIX_GLFW_LIB ${GLFW3_LIBRARIES})
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
        ${BGFX_LIB_DIR}/libexample-commonDebug.a
        ${BGFX_LIB_DIR}/libexample-glueDebug.a
    )
else()
    set(BGFX_LIBS
        ${BGFX_LIB_DIR}/libbgfxRelease.a
        ${BGFX_LIB_DIR}/libbimgRelease.a
        ${BGFX_LIB_DIR}/libbxRelease.a
        ${BGFX_LIB_DIR}/libbimg_decodeRelease.a
        ${BGFX_LIB_DIR}/libexample-commonRelease.a
        ${BGFX_LIB_DIR}/libexample-glueRelease.a
    )
endif()

# Link bgfx stack + platform libs
if(WIN32)
    target_link_libraries(openchordix_renderer PRIVATE
        ${BGFX_LIBS}
        ${OPENCHORDIX_GLFW_LIB}
    )
else()
    target_link_libraries(openchordix_renderer PRIVATE
        ${BGFX_LIBS}
        ${OPENCHORDIX_GLFW_LIB}
        GL
        X11
        dl
        pthread
    )
endif()
