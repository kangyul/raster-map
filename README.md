# raster-map

A raster tile map renderer, written from scratch in C++ and OpenGL.

This is a learning project. The goal is to understand how a slippy map actually
works — Web Mercator projection, a tile pyramid, texture upload, and the camera
math that makes panning and zooming feel right — by building one rather than
reading about it.

## Status

Early. Draws a single map tile — a PNG read from disk and uploaded as a
texture. No projection, tile addressing, or camera yet: the tile is stretched
across one fixed quad, so it does not hold its square aspect.

## Planned scope

- [x] Window and GL context (GLFW)
- [x] Render a single textured quad
- [x] Load PNG tiles from disk
- [ ] Web Mercator projection and tile addressing (z/x/y)
- [ ] Pan and zoom with a 2D camera
- [ ] Load only the tiles the viewport needs
- [ ] Tile cache with eviction
- [ ] Fetch tiles over HTTP

Tiles are read from local files to begin with, so the early work stays focused
on rendering rather than networking.

## Tiles

Tile images are not checked in — they are not ours to redistribute. Fetch the
one the program currently expects:

```bash
mkdir -p tiles/0/0
curl -A "raster-map/0.1 (learning project)" \
     -o tiles/0/0/0.png https://tile.openstreetmap.org/0/0/0.png
```

That is the whole world at zoom 0. The `-A` is not optional: OpenStreetMap's
tile policy requires a User-Agent that identifies the application, and a request
without one comes back as HTTP 200 carrying an "access blocked" image rather
than an error — so it fails silently, and the map renders the notice.

Tile data is © OpenStreetMap contributors; the public tile server is for light
use only, not bulk downloading.

## Building

Requires CMake and a C++ compiler. Dependencies are fetched automatically by
CMake, so no manual install step is needed.

```bash
cmake -B build
cmake --build build
./build/raster-map
```

The tile path is relative to the working directory, so run the binary from the
repository root rather than from `build/`.

## License

MIT
