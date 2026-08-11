# raster-map

A raster tile map renderer, written from scratch in C++ and OpenGL.

This is a learning project. The goal is to understand how a slippy map actually
works — Web Mercator projection, a tile pyramid, texture upload, and the camera
math that makes panning and zooming feel right — by building one rather than
reading about it.

## Status

Early. Nothing renders yet.

## Planned scope

- [ ] Window and GL context (GLFW)
- [ ] Render a single textured quad
- [ ] Load PNG tiles from disk
- [ ] Web Mercator projection and tile addressing (z/x/y)
- [ ] Pan and zoom with a 2D camera
- [ ] Load only the tiles the viewport needs
- [ ] Tile cache with eviction
- [ ] Fetch tiles over HTTP

Tiles are read from local files to begin with, so the early work stays focused
on rendering rather than networking.

## Building

Requires CMake and a C++ compiler. Dependencies are fetched automatically by
CMake, so no manual install step is needed.

```bash
cmake -B build
cmake --build build
./build/raster-map
```

## License

MIT
