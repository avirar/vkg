cmake_minimum_required(VERSION 3.20)
project(vkg VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW (Linux/macOS only)
if(NOT WIN32)
    find_package(glfw3 3.3 REQUIRED)
endif()

# GLM - header-only
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLM glm)
    endif()
    if(NOT (glm_FOUND OR GLM_FOUND))
        find_path(GLM_INCLUDE_DIRS glm/glm.hpp)
    endif()
endif()

# Shader compiler
find_program(GLSLC glslc REQUIRED)

# Shader compilation function
function(compile_shader TARGET SHADER_SRC SHADER_DST)
    add_custom_command(
        OUTPUT ${SHADER_DST}
        COMMAND ${GLSLC} -o ${SHADER_DST} ${SHADER_SRC}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling shader ${SHADER_SRC} -> ${SHADER_DST}"
    )
    list(APPEND SHADER_SPVS ${SHADER_DST})
    set(SHADER_SPVS ${SHADER_SPVS} PARENT_SCOPE)
endfunction()

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

# Compile shaders to SPIR-V
compile_shader(quad_vert ${SHADER_DIR}/quad.vert ${SHADER_OUT_DIR}/quad.vert.spv)
compile_shader(quad_frag ${SHADER_DIR}/quad.frag ${SHADER_OUT_DIR}/quad.frag.spv)
compile_shader(sun_vert ${SHADER_DIR}/sun.vert ${SHADER_OUT_DIR}/sun.vert.spv)
compile_shader(sun_frag ${SHADER_DIR}/sun.frag ${SHADER_OUT_DIR}/sun.frag.spv)
compile_shader(particle_comp ${SHADER_DIR}/particle.comp ${SHADER_OUT_DIR}/particle.comp.spv)

# Core source files (shared between platforms)
set(VKG_CORE
    src/config.cpp
    src/engine.cpp
    src/render.cpp
    src/compute.cpp
    src/simulation.cpp
    src/textures.cpp
    src/audio.cpp
)

if(WIN32)
    set(VKG_SOURCES ${VKG_CORE} src/screensaver.cpp ${SHADER_SPVS})
else()
    set(VKG_SOURCES ${VKG_CORE} src/main.cpp ${SHADER_SPVS})
endif()

if(WIN32)
    add_executable(vkg WIN32 ${VKG_SOURCES})
    set_target_properties(vkg PROPERTIES
        SUFFIX ".scr"
        WIN32_EXECUTABLE TRUE
    )
    target_compile_definitions(vkg PRIVATE VK_USE_PLATFORM_WIN32_KHR _WIN32_WINNT=0x0601)
else()
    add_executable(vkg ${VKG_SOURCES})
endif()

target_include_directories(vkg PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${Vulkan_INCLUDE_DIRS}
)

if(glm_FOUND)
    target_include_directories(vkg PRIVATE ${glm_INCLUDE_DIRS})
elseif(GLM_FOUND AND GLM_INCLUDE_DIRS)
    target_include_directories(vkg PRIVATE ${GLM_INCLUDE_DIRS})
elseif(GLM_INCLUDE_DIRS)
    target_include_directories(vkg PRIVATE ${GLM_INCLUDE_DIRS})
endif()

if(WIN32)
    target_link_libraries(vkg PRIVATE Vulkan::Vulkan gdi32)
else()
    target_link_libraries(vkg PRIVATE Vulkan::Vulkan glfw)
endif()

# Copy assets near the executable so they can be found at runtime
add_custom_command(TARGET vkg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_OUT_DIR}
    ${CMAKE_BINARY_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/../glg-test/glg.wav
    ${CMAKE_BINARY_DIR}/glg.wav
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/vkg.ini
    ${CMAKE_BINARY_DIR}/vkg.ini
)
