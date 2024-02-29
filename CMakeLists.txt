cmake_minimum_required(VERSION 3.27)
project(ShaderBeams LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Executable
add_executable(${PROJECT_NAME}
        shader_buffers.cpp
        shader_drawer.cpp
        main.cpp
)

# Solver library
add_subdirectory(Solver)
target_link_libraries(${PROJECT_NAME} PUBLIC Solver)
target_include_directories(${PROJECT_NAME} PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/Solver"
)

# SFML
include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics)

if(WIN32)
    add_custom_command(
            TARGET ${PROJECT_NAME}
            COMMENT "Copy OpenAL DLL"
            PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            VERBATIM)
endif()

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARY})

# GLEW
FetchContent_Declare(GLEW
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
        GIT_TAG glew-cmake-2.2.0
)
FetchContent_MakeAvailable(GLEW)
target_link_libraries(${PROJECT_NAME} PRIVATE libglew_static)

# ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.90
)
FetchContent_MakeAvailable(imgui)

#ImGui-SFML
FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml
        GIT_TAG v2.6
)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_SFML_IMGUI_DEMO ON)
FetchContent_MakeAvailable(imgui-sfml)
target_link_libraries(${PROJECT_NAME} PRIVATE ImGui-SFML::ImGui-SFML)

#imgui-filebrowser
FetchContent_Declare(
        imgui-filebrowser
        GIT_REPOSITORY https://github.com/AirGuanZ/imgui-filebrowser
        GIT_TAG a3d0f5053ab8dd1915e3fe9bc01978e76bbae5c5
)
FetchContent_MakeAvailable(imgui-filebrowser)
target_include_directories(${PROJECT_NAME} PUBLIC ${imgui-filebrowser_SOURCE_DIR})

#jsoncpp
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Shader files (copying to working directory)
add_custom_command(
        OUTPUT copy_GLSL_shaders
        COMMAND cmake -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Solver/shaders ${CMAKE_BINARY_DIR}/bin/shaders
)
add_custom_target(
        copy_GLSL_shaders_target
        DEPENDS copy_GLSL_shaders
)
add_dependencies(${PROJECT_NAME} copy_GLSL_shaders_target)

# Python files
add_custom_command(
        OUTPUT copy_Python_tools
        COMMAND cmake -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/python-tools ${CMAKE_BINARY_DIR}/bin/python-tools
)
add_custom_target(
        copy_Python_tools_target
        DEPENDS copy_Python_tools
)
add_dependencies(${PROJECT_NAME} copy_Python_tools_target)