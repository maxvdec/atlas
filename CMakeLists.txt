cmake_minimum_required(VERSION 3.15)
project(Atlas LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/core/shaders.h")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

file(GLOB SHADER_SOURCES
    ${SHADER_INPUT_DIR}/*.vert
    ${SHADER_INPUT_DIR}/*.frag
    ${SHADER_INPUT_DIR}/*.glsl
)

add_custom_target(GenerateShadersHeader ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE}
    COMMENT "Packing shaders into shaders.hpp"
    VERBATIM
)

add_compile_definitions(GL_SILENCE_DEPRECATION)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB_RECURSE ATLAS_SOURCES "atlas/*.cpp" "external/*.c" "external/*.cpp")

file(GLOB_RECURSE GLAD_SOURCES "external/glad/glad.c")

add_library(atlas STATIC ${ATLAS_SOURCES} ${GLAD_SOURCES})
add_dependencies(atlas GenerateShadersHeader)

target_link_libraries(atlas
    PRIVATE
        OpenGL::GL
        glfw
)

target_include_directories(atlas
    PRIVATE
    ${GLM_INCLUDE_DIRS}
    include
    external
)

file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")

add_executable(atlas_test ${TEST_SOURCES})

target_link_libraries(atlas_test PRIVATE atlas)

target_include_directories(atlas_test
    PRIVATE
    include
)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
