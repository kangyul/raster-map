cmake_minimum_required(VERSION 3.20)

project(RasterMap LANGUAGES CXX)

# Emit build/compile_commands.json so editors and clang-tidy see the exact
# flags each file is compiled with — including headers fetched into build/.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(stb)

find_package(OpenGL REQUIRED)

add_library(map-core OBJECT
  src/shader.cpp
  src/mercator.cpp
  src/stb_image_impl.cpp
)

target_include_directories(map-core PUBLIC ${stb_SOURCE_DIR})
target_compile_features(map-core PUBLIC cxx_std_20)
target_compile_definitions(map-core PUBLIC GL_SILENCE_DEPRECATION)
target_compile_options(map-core PUBLIC -Wall -Wextra)
target_link_libraries(map-core PUBLIC glfw OpenGL::GL)

add_executable(raster-map src/main.cpp)
target_link_libraries(raster-map PRIVATE map-core)

# Optional sandbox for experiments: `cp src/main.cpp src/scratch.cpp` and break
# things freely. Untracked, so the target only exists when the file does.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/scratch.cpp)
  add_executable(scratch src/scratch.cpp)
  target_link_libraries(scratch PRIVATE map-core)
endif()