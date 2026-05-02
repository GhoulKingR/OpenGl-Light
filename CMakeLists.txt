cmake_minimum_required(VERSION 4.0)
project(lighting)

find_library(COCOA_LIB Cocoa)
find_library(IOKIT_LIB IOKit)

find_library(OPENGL_LIB OpenGL)
if (NOT OPENGL_LIB)
    message(FATAL_ERROR ":: OpenGL Library not found. Aborting")
else()
    message(INFO ":: OpenGL Library found!")
endif()

find_library(GLFW_LIB NAMES glfw)
if (NOT GLFW_LIB)
    message(FATAL_ERROR ":: GLFW Library not found. Aborting")
else()
    message(INFO ":: GLFW Library found!")
endif()
find_package(glfw3 3.4 REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/glad.c)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIB}
    ${GLFW_LIB}
    # "/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.3.4.dylib"
    ${COCOA_LIB}
    ${IOKIT_LIB}
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "/opt/homebrew/include"
)
