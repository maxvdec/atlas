cmake_minimum_required(VERSION 3.15)
project(Atlas LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_definitions(-DOPENGL)

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

set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders")
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

add_custom_command(
    OUTPUT ${SHADER_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE}
    DEPENDS ${SHADER_SOURCES}
    COMMENT "Packing shaders into shaders.h"
)

find_package(OpenGL REQUIRED)
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

file(GLOB_RECURSE BEZEL_SOURCES bezel/*.cpp)
file(GLOB_RECURSE FINEWAVE_SOURCES finewave/*.cpp)
file(GLOB_RECURSE AURORA_SOURCES aurora/*.cpp)
file(GLOB_RECURSE HYDRA_SOURCES hydra/*.cpp)
file(GLOB_RECURSE ATLAS_SOURCES atlas/*.cpp extern/*.c)
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
file(GLOB_RECURSE OPAL_SOURCES opal/*.cpp)

add_library(bezel ${BEZEL_SOURCES})
target_link_libraries(bezel PRIVATE glfw glm::glm)

add_library(finewave ${FINEWAVE_SOURCES})
target_link_libraries(finewave PRIVATE glfw glm::glm ${OPENAL_LIB})
target_include_directories(finewave PRIVATE ${OPENAL_INCLUDE})

add_library(opal ${OPAL_SOURCES})
target_link_libraries(opal PRIVATE glm::glm glfw OpenGL::GL)

add_library(aurora ${AURORA_SOURCES})
target_link_libraries(aurora PRIVATE glm::glm glfw OpenGL::GL) 

add_library(hydra ${HYDRA_SOURCES})
target_link_libraries(hydra PRIVATE glm::glm glfw OpenGL::GL)

add_library(atlas ${ATLAS_SOURCES} ${SHADER_OUTPUT_FILE})
target_link_libraries(atlas
    PUBLIC
        OpenGL::GL
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
target_include_directories(atlas PRIVATE ${FREETYPE_INCLUDE_DIRS}/freetype2)
target_compile_definitions(atlas PRIVATE ATLAS_LIB GL_SILENT_DEPRECATION)

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
    target_link_libraries(atlas PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
elseif(UNIX)
    target_link_libraries(atlas PRIVATE dl pthread)
endif()
