cmake_minimum_required(VERSION 3.27)
project(Solver LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    set(BUILD_SHARED_LIBS TRUE)
endif ()

add_library(${PROJECT_NAME} SHARED
    Solver.cpp
)

# Shader files (copying to working directory)
add_custom_command(
        OUTPUT copy_GLSL_shaders
        COMMAND cmake -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/shaders ${CMAKE_BINARY_DIR}/bin/shaders
)
add_custom_target(
        copy_GLSL_shaders_target
        DEPENDS copy_GLSL_shaders
)
add_dependencies(${PROJECT_NAME} copy_GLSL_shaders_target)