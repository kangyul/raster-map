cmake_minimum_required(VERSION 3.20)

project(RasterMap LANGUAGES CXX)

# Emit build/compile_commands.json so editors and clang-tidy see the exact
# flags each file is compiled with — including headers fetched into build/.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(raster-map)

target_sources(raster-map
  PRIVATE
    src/main.cpp src/shader.cpp
)

target_compile_features(raster-map PRIVATE cxx_std_20)
target_compile_definitions(raster-map PRIVATE GL_SILENCE_DEPRECATION)

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

# Optional sandbox for experiments: `cp src/main.cpp src/scratch.cpp` and break
# things freely. Untracked, so the target only exists when the file does.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/scratch.cpp)
  add_executable(scratch)
  target_sources(scratch PRIVATE src/scratch.cpp)
  target_compile_features(scratch PRIVATE cxx_std_20)
  target_link_libraries(scratch PRIVATE glfw OpenGL::GL)
endif()

target_link_libraries(raster-map PRIVATE glfw OpenGL::GL)