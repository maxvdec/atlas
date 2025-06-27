cmake_minimum_required(VERSION 3.15)
project(Atlas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB ATLAS_SOURCES "atlas/*.cpp")

add_library(atlas STATIC ${ATLAS_SOURCES})

target_link_libraries(atlas
    PRIVATE
        OpenGL::GL
        glfw
)

target_include_directories(atlas
    PRIVATE
    ${GLM_INCLUDE_DIRS}
    include
)

file(GLOB TEST_SOURCES "test/*.cpp")

add_executable(atlas_test ${TEST_SOURCES})

target_link_libraries(atlas_test PRIVATE atlas)

target_include_directories(atlas_test
    PRIVATE
    include
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
