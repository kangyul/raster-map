cmake_minimum_required(VERSION 3.20)

project(RasterMap LANGUAGES CXX)

add_executable(raster-map)

target_sources(raster-map
  PRIVATE
    src/main.cpp
)

target_compile_features(raster-map PRIVATE cxx_std_20)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.5.1
  GIT_SHALLOW    TRUE
)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

target_link_libraries(raster-map PRIVATE glfw OpenGL::GL)