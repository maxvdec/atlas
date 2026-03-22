cmake_minimum_required(VERSION 3.15)
project(Atlas LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JOLT_VERSION "5.5.0")

message("Using Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

add_definitions(-DATLAS_VERSION="Alpha 9")
add_definitions(-DOPAL_VERSION="3 Tetrahedron")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# ─── Compiler Diagnostics ────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-gnu-anonymous-struct)
endif()

# ─── Build Type ──────────────────────────────────────────────────────────────
message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ─── Output Directories ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── Rendering Backend ───────────────────────────────────────────────────────
set(BACKEND "AUTO" CACHE STRING "Rendering backend: AUTO, VULKAN, METAL, OPENGL")
set_property(CACHE BACKEND PROPERTY STRINGS AUTO VULKAN METAL OPENGL)
string(TOUPPER "${BACKEND}" BACKEND)

if(BACKEND STREQUAL "AUTO")
    if(APPLE)
        set(BACKEND "METAL")
    else()
        set(BACKEND "VULKAN")
    endif()
endif()

set(BACKEND_OPENGL OFF)
set(BACKEND_VULKAN OFF)
set(BACKEND_METAL  OFF)

if(BACKEND STREQUAL "OPENGL")
    set(BACKEND_OPENGL ON)
elseif(BACKEND STREQUAL "VULKAN")
    set(BACKEND_VULKAN ON)
elseif(BACKEND STREQUAL "METAL")
    set(BACKEND_METAL ON)
else()
    message(FATAL_ERROR "Invalid BACKEND='${BACKEND}'. Valid values are: AUTO, VULKAN, METAL, OPENGL")
endif()

message(STATUS "Selected backend: ${BACKEND}")

# ─── Options ─────────────────────────────────────────────────────────────────
option(BEZEL_NATIVE "Enable Bezel Native" OFF)

# ─── FetchContent ────────────────────────────────────────────────────────────
include(FetchContent)
cmake_policy(SET CMP0135 NEW)

FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.4.0.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

if(NOT BEZEL_NATIVE)
    FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG        v${JOLT_VERSION}
        GIT_SHALLOW    TRUE
        SOURCE_SUBDIR  Build
    )
    set(JPH_DEBUG_RENDERER  ON  CACHE BOOL "" FORCE)
    set(JPH_PROFILE_ENABLED ON  CACHE BOOL "" FORCE)
    set(JPH_OBJECT_STREAM   ON  CACHE BOOL "" FORCE)
    set(JPH_ENABLE_IPO      OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(JoltPhysics)
    if(TARGET Jolt)
        set_target_properties(Jolt PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
    endif()
    message(STATUS "Downloaded Jolt Physics, version ${JOLT_VERSION}")
endif()

# ─── Backend Definitions ─────────────────────────────────────────────────────
if(BACKEND_OPENGL)
    add_definitions(-DOPENGL)
    message(STATUS "Building using OpenGL backend")
elseif(BACKEND_VULKAN)
    add_definitions(-DVULKAN)
    add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
    message(STATUS "Building using Vulkan backend")
elseif(BACKEND_METAL)
    add_definitions(-DMETAL)
    add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
    message(STATUS "Building using Metal backend")
endif()

# ─── Shader Generation (runs before everything else) ─────────────────────────
if(BACKEND_OPENGL)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/opengl")
elseif(BACKEND_VULKAN)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/vulkan")
elseif(BACKEND_METAL)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/metal")
endif()

set(SHADER_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/core/default_shaders.h")
set(RUNTIME_SCRIPT_INPUT_DIR "${CMAKE_SOURCE_DIR}/runtime/scripts")
set(RUNTIME_SCRIPT_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/runtime/atlasScripts.h")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

file(GLOB_RECURSE SHADER_SOURCES
    ${SHADER_INPUT_DIR}/*.vert
    ${SHADER_INPUT_DIR}/*.frag
    ${SHADER_INPUT_DIR}/*.glsl
    ${SHADER_INPUT_DIR}/*.geom
    ${SHADER_INPUT_DIR}/*.tesc
    ${SHADER_INPUT_DIR}/*.tese
    ${SHADER_INPUT_DIR}/*.metal
)

file(GLOB_RECURSE RUNTIME_SCRIPT_SOURCES CONFIGURE_DEPENDS
    ${RUNTIME_SCRIPT_INPUT_DIR}/*.js
    ${RUNTIME_SCRIPT_INPUT_DIR}/*.mjs
)

if(BACKEND_OPENGL)
    set(SHADER_BACKEND_ARG "opengl")
elseif(BACKEND_VULKAN)
    set(SHADER_BACKEND_ARG "vulkan")
elseif(BACKEND_METAL)
    set(SHADER_BACKEND_ARG "metal")
endif()

message(STATUS "Packing shaders for ${BACKEND} backend")
add_custom_command(
    OUTPUT  ${SHADER_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py
                ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE} ${SHADER_BACKEND_ARG}
    DEPENDS ${SHADER_SOURCES}
    COMMENT "Packing shaders into default_shaders.h"
    VERBATIM
)

# This named target is what everything else depends on to force ordering.
add_custom_target(generate_shaders
    DEPENDS ${SHADER_OUTPUT_FILE}
)

add_custom_command(
    OUTPUT  ${RUNTIME_SCRIPT_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/runtime
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_runtime_scripts.py
                ${RUNTIME_SCRIPT_INPUT_DIR} ${RUNTIME_SCRIPT_OUTPUT_FILE}
    DEPENDS ${RUNTIME_SCRIPT_SOURCES} ${CMAKE_SOURCE_DIR}/pack_runtime_scripts.py
    COMMENT "Packing runtime scripts into atlasScripts.h"
    VERBATIM
)

add_custom_target(generate_runtime_scripts
    DEPENDS ${RUNTIME_SCRIPT_OUTPUT_FILE}
)

# ─── Typst Docs ──────────────────────────────────────────────────────────────
find_program(TYPST_EXECUTABLE typst)
if(TYPST_EXECUTABLE)
    message(STATUS "Found Typst: ${TYPST_EXECUTABLE}")
    file(GLOB_RECURSE TYPST_SOURCES ${CMAKE_SOURCE_DIR}/*.typ)
    if(TYPST_SOURCES)
        set(TYPST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)
        file(MAKE_DIRECTORY ${TYPST_OUTPUT_DIR})
        set(TYPST_OUTPUTS "")
        foreach(TYPST_FILE ${TYPST_SOURCES})
            get_filename_component(TYPST_NAME ${TYPST_FILE} NAME_WE)
            set(PDF_OUTPUT ${TYPST_OUTPUT_DIR}/${TYPST_NAME}.pdf)
            add_custom_command(
                OUTPUT  ${PDF_OUTPUT}
                COMMAND ${TYPST_EXECUTABLE} compile ${TYPST_FILE} ${PDF_OUTPUT}
                DEPENDS ${TYPST_FILE}
                COMMENT "Compiling Typst document: ${TYPST_NAME}.typ -> ${TYPST_NAME}.pdf"
                VERBATIM
            )
            list(APPEND TYPST_OUTPUTS ${PDF_OUTPUT})
        endforeach()
        add_custom_target(typst_docs ALL DEPENDS ${TYPST_OUTPUTS})
        message(STATUS "Typst documents will be compiled to: ${TYPST_OUTPUT_DIR}")
    else()
        message(STATUS "No Typst documents found")
    endif()
else()
    message(WARNING "Typst not found - skipping document compilation")
    message(WARNING "Install Typst from https://typst.app to enable document building")
endif()

# ─── Find Packages ───────────────────────────────────────────────────────────
if(BACKEND_OPENGL)
    find_package(OpenGL REQUIRED)
elseif(BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan SDK at ${Vulkan_LIBRARY}")
    message(STATUS "Found Vulkan include dirs at ${Vulkan_INCLUDE_DIRS}")
    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/spirv-cross")
    endif()
    find_package(spirv_cross_core    REQUIRED)
    find_package(spirv_cross_glsl    REQUIRED)
    find_package(spirv_cross_reflect REQUIRED)
elseif(BACKEND_METAL)
    message(STATUS "Linking Metal to source")
endif()

find_package(glm      REQUIRED)
find_package(Freetype REQUIRED)
find_package(Assimp   REQUIRED)
find_package(SDL3     REQUIRED CONFIG)

if(APPLE)
    set(OPENAL_LIB     /opt/homebrew/opt/openal-soft/lib/libopenal.dylib)
    set(OPENAL_INCLUDE /opt/homebrew/opt/openal-soft/include)
else()
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIB     ${OPENAL_LIBRARY})
    set(OPENAL_INCLUDE ${OPENAL_INCLUDE_DIR})
endif()

# ─── Global Includes ─────────────────────────────────────────────────────────
include_directories(include extern ${OPENAL_INCLUDE})

if(NOT BEZEL_NATIVE)
    include_directories(${joltphysics_SOURCE_DIR})
endif()

# ─── Sources ─────────────────────────────────────────────────────────────────
if(BEZEL_NATIVE)
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/native/*.cpp)
    message(STATUS "Compiling with Bezel Native")
else()
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/jolt/*.cpp)
    message(STATUS "Compiling with Bezel Jolt (Jolt Physics ${JOLT_VERSION})")
endif()

file(GLOB_RECURSE FINEWAVE_SOURCES CONFIGURE_DEPENDS finewave/*.cpp)
file(GLOB_RECURSE AURORA_SOURCES   CONFIGURE_DEPENDS aurora/*.cpp)
file(GLOB_RECURSE HYDRA_SOURCES    CONFIGURE_DEPENDS hydra/*.cpp)
file(GLOB_RECURSE ATLAS_SOURCES    CONFIGURE_DEPENDS atlas/*.cpp extern/glad/*.c)
file(GLOB_RECURSE OPAL_SOURCES     CONFIGURE_DEPENDS opal/*.cpp)
file(GLOB_RECURSE PHOTON_SOURCES   CONFIGURE_DEPENDS photon/*.cpp)
file(GLOB_RECURSE GRAPHITE_SOURCES CONFIGURE_DEPENDS graphite/*.cpp)
file(GLOB_RECURSE RUNTIME_LIB_SOURCES CONFIGURE_DEPENDS runtime/lib/*.cpp)
file(GLOB_RECURSE RUNTIME_SOURCES CONFIGURE_DEPENDS runtime/executable/*.cpp)
file(GLOB_RECURSE QUICKJS_SOURCES CONFIGURE_DEPENDS extern/quickjs/*.c)

# ─── bezel ───────────────────────────────────────────────────────────────────
add_library(bezel ${BEZEL_SOURCES})
target_link_libraries(bezel PRIVATE SDL3::SDL3 glm::glm)
if(BEZEL_NATIVE)
    target_compile_definitions(bezel PRIVATE NATIVE)
else()
    target_compile_definitions(bezel PRIVATE
        JPH_PROFILE_ENABLED=1
        JPH_DEBUG_RENDERER=1
        JPH_OBJECT_STREAM=1
    )
endif()

# ─── finewave ────────────────────────────────────────────────────────────────
add_library(finewave ${FINEWAVE_SOURCES})
target_link_libraries(finewave PRIVATE SDL3::SDL3 glm::glm ${OPENAL_LIB})
target_include_directories(finewave PRIVATE ${OPENAL_INCLUDE})

# ─── opal ────────────────────────────────────────────────────────────────────
add_library(opal ${OPAL_SOURCES})
target_link_libraries(opal PRIVATE glm::glm SDL3::SDL3)
add_dependencies(opal generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(opal PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(opal PRIVATE Vulkan::Vulkan spirv-cross-core spirv-cross-glsl spirv-cross-reflect)
    target_compile_definitions(opal PRIVATE VULKAN)
endif()

# ─── photon ──────────────────────────────────────────────────────────────────
add_library(photon ${PHOTON_SOURCES})
target_link_libraries(photon PRIVATE opal glm::glm)
add_dependencies(photon generate_shaders)

# ─── aurora ──────────────────────────────────────────────────────────────────
add_library(aurora ${AURORA_SOURCES})
target_link_libraries(aurora PRIVATE glm::glm SDL3::SDL3 bezel)
add_dependencies(aurora generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(aurora PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(aurora PRIVATE Vulkan::Vulkan)
endif()

# ─── hydra ───────────────────────────────────────────────────────────────────
add_library(hydra ${HYDRA_SOURCES})
target_link_libraries(hydra PRIVATE glm::glm SDL3::SDL3 bezel)
add_dependencies(hydra generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(hydra PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(hydra PRIVATE Vulkan::Vulkan)
endif()

# ─── graphite ───────────────────────────────────────────────────────────────────
add_library(graphite ${GRAPHITE_SOURCES})
target_link_libraries(graphite PRIVATE glm::glm SDL3::SDL3 Freetype::Freetype assimp::assimp)
add_dependencies(graphite generate_shaders)

# ─── atlas ───────────────────────────────────────────────────────────────────
add_library(atlas ${ATLAS_SOURCES})
add_dependencies(atlas generate_shaders)

target_link_libraries(atlas
    PUBLIC
        SDL3::SDL3
        glm::glm
        bezel
        finewave
        aurora
        hydra
        opal
        Freetype::Freetype
        assimp::assimp
        photon
        graphite
)

if(NOT BEZEL_NATIVE)
    target_link_libraries(atlas PUBLIC Jolt::Jolt)
endif()

if(BACKEND_OPENGL)
    target_link_libraries(atlas PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(atlas PRIVATE Vulkan::Vulkan)
    if(APPLE)
        message(STATUS "Configuring MoltenVK for macOS")
        target_compile_definitions(atlas PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    endif()
endif()

target_include_directories(atlas PRIVATE ${FREETYPE_INCLUDE_DIRS}/freetype2)
target_compile_definitions(atlas PRIVATE ATLAS_LIB GL_SILENT_DEPRECATION)
target_compile_options(atlas PRIVATE -Wno-overlength-strings)

if(WIN32)
    target_compile_definitions(atlas PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_link_libraries(atlas PRIVATE opengl32)
elseif(APPLE)
    if(BACKEND_OPENGL)
        target_link_libraries(atlas PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
    elseif(BACKEND_VULKAN)
        target_link_libraries(atlas PRIVATE "-framework Cocoa" "-framework IOKit" "-framework QuartzCore")
    elseif(BACKEND_METAL)
        target_link_libraries(atlas PRIVATE "-framework Metal" "-framework MetalKit" "-framework MetalFX" "-framework Cocoa" "-framework IOKit" "-framework QuartzCore")
    endif()
elseif(UNIX)
    target_link_libraries(atlas PRIVATE dl pthread)
endif()

# ─── quickjs ──────────────────────────────────────────────────────────────
add_library(quickjs STATIC ${QUICKJS_SOURCES})

target_include_directories(quickjs PUBLIC
    ${CMAKE_SOURCE_DIR}/extern/quickjs
)

if(MSVC)
    target_compile_options(quickjs PRIVATE /W0)
else()
    target_compile_options(quickjs PRIVATE
        -w
        -Wno-error
    )
endif()

# ─── runtime_lib ──────────────────────────────────────────────────────────────
add_library(runtime_lib ${RUNTIME_LIB_SOURCES})
target_link_libraries(runtime_lib PRIVATE atlas quickjs)
target_compile_definitions(runtime_lib PRIVATE RUNTIME_LIB JPH_PROFILE_ENABLED=1 JPH_DEBUG_RENDERER=1 JPH_OBJECT_STREAM=1)
add_dependencies(runtime_lib generate_shaders generate_runtime_scripts)
target_include_directories(runtime_lib PUBLIC ${CMAKE_SOURCE_DIR}/extern/quickjs)

# ─── runtime ──────────────────────────────────────────────────────────────
add_executable(atlasrun ${RUNTIME_SOURCES})
target_link_libraries(atlasrun PRIVATE runtime_lib glm::glm)
target_compile_definitions(atlasrun PRIVATE RUNTIME_BIN JPH_PROFILE_ENABLED=1 JPH_DEBUG_RENDERER=1 JPH_OBJECT_STREAM=1)
target_include_directories(atlasrun PRIVATE ${CMAKE_SOURCE_DIR}/runtime/include)
add_dependencies(atlasrun generate_shaders)
