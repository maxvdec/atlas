cmake_minimum_required(VERSION 3.15)
project(Atlas)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


include(FetchContent)
FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.4.0.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

file(GLOB_RECURSE DOXYGEN_EXTRA_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/docs/resources/*")
string(REPLACE ";" " " DOXYGEN_HTML_EXTRA_FILES "${DOXYGEN_EXTRA_FILES_LIST}")

set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)


set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/core/default_shaders.h")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
file(GLOB_RECURSE SHADER_SOURCES
    ${SHADER_INPUT_DIR}/*.vert
    ${SHADER_INPUT_DIR}/*.frag
    ${SHADER_INPUT_DIR}/*.glsl
)
add_custom_command(
    OUTPUT ${SHADER_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE}
    DEPENDS ${SHADER_SOURCES}
    COMMENT "Packing shaders into shaders.h"
)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)
if(APPLE)
    set(OPENAL_LIB /opt/homebrew/opt/openal-soft/lib/libopenal.dylib)
    set(OPENAL_INCLUDE /opt/homebrew/opt/openal-soft/include)
else()
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIB ${OPENAL_LIBRARY}
        OPENAL_INCLUDE ${OPENAL_INCLUDE_DIR})
endif()
file(GLOB_RECURSE BEZEL_SOURCES bezel/*.cpp)
add_library(bezel ${BEZEL_SOURCES})
target_link_libraries(bezel PRIVATE glfw glm::glm)

# Finewave library
file(GLOB_RECURSE FINEWAVE_SOURCES finewave/*.cpp)
add_library(finewave ${FINEWAVE_SOURCES})
target_link_libraries(finewave PRIVATE glfw glm::glm ${OPENAL_LIB})

# Atlas library
file(GLOB_RECURSE SOURCES atlas/*.cpp extern/*.c)
add_library(atlas ${SOURCES} ${SHADER_OUTPUT_FILE})
target_link_libraries(atlas PUBLIC OpenGL::GL glfw glm::glm bezel finewave ${FREETYPE_LIBRARIES})
target_include_directories(atlas PRIVATE ${FREE_TYPE_INCLUDE_DIRS}/freetype2)
target_compile_definitions(atlas PRIVATE ATLAS_LIB GL_SILENT_DEPRECATION)

# Atlas test executable
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
get_filename_component(TEST_PATH_ABS "${CMAKE_CURRENT_SOURCE_DIR}/test" ABSOLUTE)
add_executable(atlas_test ${TEST_SOURCES})
target_link_libraries(atlas_test PRIVATE atlas)
target_compile_definitions(atlas_test
    PRIVATE
    TEST_PATH="${TEST_PATH_ABS}"
)
include_directories(include extern)
include_directories(${OPENAL_INCLUDE})