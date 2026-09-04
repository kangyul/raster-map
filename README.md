# raster-map

A raster tile map renderer, written from scratch in C++ and OpenGL.

This is a learning project. The goal is to understand how a slippy map actually
works — Web Mercator projection, a tile pyramid, texture upload, and the camera
math that makes panning and zooming feel right — by building one rather than
reading about it.

## Status

Draws a 2x2 grid of zoom-1 tiles filling the window, each placed on screen from
its z/x/y address. Web Mercator projection and tile addressing are implemented
and tested (`src/mercator.cpp`), but the renderer does not consult them yet: the
tiles on screen are a hardcoded set rather than one chosen for a viewport.

No camera. The view is fixed, and the square world is stretched to the window's
aspect ratio - both are what the next step fixes.

## Planned scope

- [x] Window and GL context (GLFW)
- [x] Render a single textured quad
- [x] Load PNG tiles from disk
- [x] Web Mercator projection and tile addressing (z/x/y)
- [ ] Pan and zoom with a 2D camera
- [ ] Load only the tiles the viewport needs
- [ ] Tile cache with eviction
- [ ] Fetch tiles over HTTP

Tiles are read from local files to begin with, so the early work stays focused
on rendering rather than networking.

## Coordinate spaces

A map renderer is mostly the business of moving a point between coordinate
spaces, and most of the bugs are two spaces mistaken for each other. The ones in
play so far:

| Space | Range | y grows | Lives in |
|---|---|---|---|
| Geographic | lon +/-180 deg, lat +/-85.0511 deg | north | `LonLat` |
| World | `[0,1]` square, origin north-west | south | `WorldPos` |
| Tile id | integers `0 .. 2^z - 1` | south | `TileId` |
| Tile-local | `[0,1]` within one tile | south | quad vertex positions |
| Texture | `[0,1]` uv | south | quad vertex uvs |
| Image | 256x256 integer pixels, row 0 is north | south | `stbi_load` buffer |
| NDC | `[-1,1]`, origin at centre | **north** | `gl_Position` |

World, tile id and tile-local describe the same point three ways: *where on
Earth* (continuous, zoom independent), *which tile* (integer, zoom dependent),
and *where inside that tile* (continuous). Tile-local and texture coordinates
hold identical numbers because a tile's top-left corner is the image's first
pixel - one space under two names.

Two chains meet at the draw call:

    where the tile goes:  LonLat -> World -> tile id + tile-local -> NDC
    what colour it is:    image pixels -> texture -> sampled by uv

Note the `y grows` column: every space runs southward except NDC. **The y axis
is flipped exactly once in the whole pipeline, in the tile-to-NDC transform.**
A second flip anywhere - `stbi_set_flip_vertically_on_load`, reversed texture
coordinates - cancels the first and lands the map upside down.

A screen space joins this list at the camera step, carrying a flip of its own:
GLFW reports cursor positions with y down, OpenGL window coordinates with y up.

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
