cmake_minimum_required(VERSION 3.20)

project(RasterMap LANGUAGES CXX)

add_executable(raster-map)

target_sources(raster-map
  PRIVATE
    src/main.cpp
)

target_compile_features(raster-map PRIVATE cxx_std_20)