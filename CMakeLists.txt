cmake_minimum_required(VERSION 3.15)
project(Atlas)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCES atlas/*.cpp extern/*.c)

add_library(atlas ${SOURCES})

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(atlas PRIVATE OpenGL::GL glfw3 glm::glm)

file(GLOB_RECURSE TEST_SOURCES test/*.cpp)

add_executable(atlas_test ${TEST_SOURCES})

include_directories(include extern)
