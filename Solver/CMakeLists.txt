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