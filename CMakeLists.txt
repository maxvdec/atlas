cmake_minimum_required(VERSION 3.15)
project(Atlas LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DATLAS_VERSION=\"Alpha\ 5\")
add_definitions(-DOPAL_VERSION=\"1.1\ Ellipse\")

message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(BACKEND_OPENGL "Enable OpenGL backend" OFF)
option(BEZEL_NATIVE "Enable Bezel Native" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.4.0.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

if(BACKEND_OPENGL)
    message(STATUS "Building using OpenGL backend")
    add_definitions(-DOPENGL)
else()
    message(STATUS "Building using Vulkan backend")
    add_definitions(-DVULKAN)
    add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
endif()

if (BACKEND_OPENGL)
    message(STATUS "Configuring shaders for OpenGL backend")
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/opengl")
else()
    message(STATUS "Configuring shaders for Vulkan backend")
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/vulkan")
endif()
set(SHADER_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/core/default_shaders.h")
find_package(Python3 COMPONENTS Interpreter REQUIRED)

file(GLOB_RECURSE SHADER_SOURCES
    ${SHADER_INPUT_DIR}/*.vert
    ${SHADER_INPUT_DIR}/*.frag
    ${SHADER_INPUT_DIR}/*.glsl
    ${SHADER_INPUT_DIR}/*.geom
    ${SHADER_INPUT_DIR}/*.tesc
    ${SHADER_INPUT_DIR}/*.tese
)

if(BACKEND_OPENGL)
    message(STATUS "Packing shaders without SPIR-V compilation")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE} false
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Packing shaders into shaders.h"
    )
else()
    message(STATUS "Packing shaders with SPIR-V compilation")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE} true
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Packing shaders into shaders.h"
    )
endif()

find_program(TYPST_EXECUTABLE typst)
if(TYPST_EXECUTABLE)
    message(STATUS "Found Typst: ${TYPST_EXECUTABLE}")

    file(GLOB_RECURSE TYPST_SOURCES ${CMAKE_SOURCE_DIR}/*.typ)

    if(TYPST_SOURCES)
        message(STATUS "Found ${CMAKE_MATCH_COUNT} Typst document(s)")
        set(TYPST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)
        file(MAKE_DIRECTORY ${TYPST_OUTPUT_DIR})

        set(TYPST_OUTPUTS "")
        foreach(TYPST_FILE ${TYPST_SOURCES})
            get_filename_component(TYPST_NAME ${TYPST_FILE} NAME_WE)
            set(PDF_OUTPUT ${TYPST_OUTPUT_DIR}/${TYPST_NAME}.pdf)

            add_custom_command(
                OUTPUT ${PDF_OUTPUT}
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

if(BEZEL_NATIVE)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG v5.4.0
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_SUBDIR Build
)
    FetchContent_MakeAvailable(JoltPhysics)
    message(STATUS "Downloaded Jolt Physics, version 5.4.0")
endif()

if(BACKEND_OPENGL)
    find_package(OpenGL REQUIRED)
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan SDK at ${Vulkan_LIBRARY}")
    message(STATUS "Found Vulkan Libraries at ${Vulkan_INCLUDE_DIRS}")

    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/spirv-cross")
    endif()
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_reflect REQUIRED)
endif()

find_package(glm REQUIRED)
find_package(Freetype REQUIRED)
find_package(Assimp REQUIRED)
find_package(glfw3 REQUIRED)

if(APPLE)
    set(OPENAL_LIB /opt/homebrew/opt/openal-soft/lib/libopenal.dylib)
    set(OPENAL_INCLUDE /opt/homebrew/opt/openal-soft/include)
elseif(WIN32)
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIB ${OPENAL_LIBRARY})
    set(OPENAL_INCLUDE ${OPENAL_INCLUDE_DIR})
elseif(UNIX)
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIB ${OPENAL_LIBRARY})
    set(OPENAL_INCLUDE ${OPENAL_INCLUDE_DIR})
endif()

if (BEZEL_NATIVE)
    message(STATUS "Compiling with Bezel Native")
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/native/*.cpp)
else()
    message(STATUS "Compiling with Bezel Jolt (Jolt Physics 5.4.0)")
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/jolt/*.cpp)
endif()

file(GLOB_RECURSE FINEWAVE_SOURCES CONFIGURE_DEPENDS finewave/*.cpp)
file(GLOB_RECURSE AURORA_SOURCES CONFIGURE_DEPENDS aurora/*.cpp)
file(GLOB_RECURSE HYDRA_SOURCES CONFIGURE_DEPENDS hydra/*.cpp)
file(GLOB_RECURSE ATLAS_SOURCES CONFIGURE_DEPENDS atlas/*.cpp extern/*.c)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS test/*.cpp)
file(GLOB_RECURSE OPAL_SOURCES CONFIGURE_DEPENDS opal/*.cpp)

add_library(bezel ${BEZEL_SOURCES})
target_link_libraries(bezel PRIVATE glfw glm::glm)
if (BEZEL_NATIVE)
    target_compile_definitions(bezel PRIVATE NATIVE)
else()
    target_link_libraries(bezel PUBLIC Jolt::Jolt)
endif()

add_library(finewave ${FINEWAVE_SOURCES})
target_link_libraries(finewave PRIVATE glfw glm::glm ${OPENAL_LIB})
target_include_directories(finewave PRIVATE ${OPENAL_INCLUDE})

add_library(opal ${OPAL_SOURCES})
target_link_libraries(opal PRIVATE glm::glm glfw)

if (BACKEND_OPENGL)
    target_link_libraries(opal PRIVATE OpenGL::GL)
else()
    target_link_libraries(opal PRIVATE Vulkan::Vulkan spirv-cross-core spirv-cross-glsl spirv-cross-reflect)
    target_compile_definitions(opal PRIVATE VULKAN)
endif()

add_library(aurora ${AURORA_SOURCES})
target_link_libraries(aurora PRIVATE glm::glm glfw bezel)

if (BACKEND_OPENGL)
    target_link_libraries(aurora PRIVATE OpenGL::GL)
else()
    target_link_libraries(aurora PRIVATE Vulkan::Vulkan)
endif()

add_library(hydra ${HYDRA_SOURCES})
target_link_libraries(hydra PRIVATE glm::glm glfw bezel)

if (BACKEND_OPENGL)
    target_link_libraries(hydra PRIVATE OpenGL::GL)
else()
    target_link_libraries(hydra PRIVATE Vulkan::Vulkan)
endif()

add_library(atlas ${ATLAS_SOURCES} ${SHADER_OUTPUT_FILE})
target_link_libraries(atlas
    PUBLIC
        glfw
        glm::glm
        bezel
        finewave
        aurora
        hydra
        opal
        ${FREETYPE_LIBRARIES}
        assimp::assimp
)

if(BEZEL_NATIVE)
else()
    include_directories(${JoltPhysics_SOURCE_DIR})
endif()

if(BACKEND_OPENGL)
    target_link_libraries(atlas PRIVATE OpenGL::GL)
else()
    target_link_libraries(atlas PRIVATE Vulkan::Vulkan)

    if(APPLE)
        message(STATUS "Configuring MoltenVK for macOS")
        target_compile_definitions(atlas PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    endif()
endif()

target_include_directories(atlas PRIVATE ${FREETYPE_INCLUDE_DIRS}/freetype2)
target_compile_definitions(atlas PRIVATE ATLAS_LIB GL_SILENT_DEPRECATION)

target_compile_options(atlas PRIVATE
    -Wno-overlength-strings
)

get_filename_component(TEST_PATH_ABS "${CMAKE_CURRENT_SOURCE_DIR}/test" ABSOLUTE)
add_executable(atlas_test ${TEST_SOURCES})
target_link_libraries(atlas_test PRIVATE atlas)
target_compile_definitions(atlas_test PRIVATE TEST_PATH="${TEST_PATH_ABS}")

include_directories(include extern)
include_directories(${OPENAL_INCLUDE})

if(WIN32)
    target_compile_definitions(atlas PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_link_libraries(atlas PRIVATE opengl32)
elseif(APPLE)
    if(BACKEND_OPENGL)
        target_link_libraries(atlas PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
    else()
        target_link_libraries(atlas PRIVATE "-framework Cocoa" "-framework IOKit" "-framework QuartzCore")
    endif()
elseif(UNIX)
    target_link_libraries(atlas PRIVATE dl pthread)
endif()
