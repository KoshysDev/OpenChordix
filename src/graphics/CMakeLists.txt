set(BGFX_ROOT ${CMAKE_SOURCE_DIR}/external)
set(BX_DIR ${BGFX_ROOT}/bx)
set(BIMG_DIR ${BGFX_ROOT}/bimg)
set(BGFX_DIR ${BGFX_ROOT}/bgfx)

add_library(openchordix_renderer STATIC
    Renderer.cpp
    gltf/GltfLoader.cpp
    render/Model.cpp
    render/ModelBuilder.cpp
    render/ModelRenderer.cpp
    render/ShaderLoader.cpp
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
    ${CMAKE_SOURCE_DIR}/src/app
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

set(OPENCHORDIX_GENERATED_ASSET_ROOT "${CMAKE_BINARY_DIR}/generated-assets" CACHE INTERNAL "OpenChordix generated assets root")
set(OPENCHORDIX_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(OPENCHORDIX_SHADER_OUTPUT_DIR "${OPENCHORDIX_GENERATED_ASSET_ROOT}/shaders")

find_program(OPENCHORDIX_SHADERC shadercRelease
    PATHS ${BGFX_DIR}/.build/linux64_gcc/bin
    NO_DEFAULT_PATH)

add_custom_target(OpenChordixShaders)

if(OPENCHORDIX_SHADERC)
    set(STANDARD_SHADER_DIR "${OPENCHORDIX_SHADER_DIR}/standard")
    set(STANDARD_VARYING_DEF "${STANDARD_SHADER_DIR}/varying.def.sc")
    set(STANDARD_VS_SOURCE "${STANDARD_SHADER_DIR}/vs_standard.sc")
    set(STANDARD_FS_SOURCE "${STANDARD_SHADER_DIR}/fs_standard.sc")

    function(openchordix_compile_shader out_var shader_type source_file)
        set(outputs "")
        foreach(backend IN ITEMS glsl spirv)
            if(backend STREQUAL "glsl")
                set(platform linux)
                set(profile 120)
            else()
                set(platform linux)
                set(profile spirv)
            endif()
            get_filename_component(source_name "${source_file}" NAME_WE)
            set(output_dir "${OPENCHORDIX_SHADER_OUTPUT_DIR}/${backend}")
            set(output_file "${output_dir}/${source_name}.bin")
            add_custom_command(
                OUTPUT "${output_file}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
                COMMAND "${OPENCHORDIX_SHADERC}"
                    -f "${source_file}"
                    -o "${output_file}"
                    --type "${shader_type}"
                    --platform "${platform}"
                    -p "${profile}"
                    --varyingdef "${STANDARD_VARYING_DEF}"
                    -i "${BGFX_DIR}/src"
                    -i "${BGFX_DIR}/examples/common"
                    -i "${STANDARD_SHADER_DIR}"
                DEPENDS "${source_file}" "${STANDARD_VARYING_DEF}"
                VERBATIM
            )
            list(APPEND outputs "${output_file}")
        endforeach()
        set(${out_var} "${outputs}" PARENT_SCOPE)
    endfunction()

    openchordix_compile_shader(STANDARD_VS_OUTPUTS vertex "${STANDARD_VS_SOURCE}")
    openchordix_compile_shader(STANDARD_FS_OUTPUTS fragment "${STANDARD_FS_SOURCE}")
    set(STANDARD_SHADER_OUTPUTS ${STANDARD_VS_OUTPUTS} ${STANDARD_FS_OUTPUTS})
    add_custom_target(OpenChordixStandardShaders DEPENDS ${STANDARD_SHADER_OUTPUTS})
    add_dependencies(OpenChordixShaders OpenChordixStandardShaders)
    set(OPENCHORDIX_SHADER_OUTPUTS ${STANDARD_SHADER_OUTPUTS} CACHE INTERNAL "OpenChordix shader binaries")
else()
    message(WARNING "shadercRelease not found; OpenChordix shaders will not be compiled.")
endif()
