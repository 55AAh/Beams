cmake_minimum_required(VERSION 3.27)
project(ShaderBeams LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Executable
add_executable(${PROJECT_NAME} main.cpp)

# Solver library
add_subdirectory(Solver)
target_link_libraries(${PROJECT_NAME} PUBLIC Solver)
target_include_directories(${PROJECT_NAME} PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/Solver"
)
