# raster-map

A raster tile map renderer, written from scratch in C++ and OpenGL.

This is a learning project. The goal is to understand how a slippy map actually
works — Web Mercator projection, a tile pyramid, texture upload, and the camera
math that makes panning and zooming feel right — by building one rather than
reading about it.

## Status

Draws a 2x2 grid of zoom-1 tiles through a 2D camera. The camera is a world
position to centre on plus a continuous zoom level, and `tileToNDC` places each
tile from its z/x/y address by way of that camera — so the map now holds its
square shape in a window of any proportion, where before it stretched.

No input yet: the camera's values are fixed in code, and the tiles on screen are
a hardcoded set rather than one chosen for a viewport. Web Mercator projection
and tile addressing are implemented in `src/mercator.cpp` — not yet under test,
and not yet consulted by the renderer.

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
| Screen | pixels, origin at the window centre | south | inside `tileToNDC` |
| NDC | `[-1,1]`, origin at centre | **north** | `gl_Position` |

World, tile id and tile-local describe the same point three ways: *where on
Earth* (continuous, zoom independent), *which tile* (integer, zoom dependent),
and *where inside that tile* (continuous). Tile-local and texture coordinates
hold identical numbers because a tile's top-left corner is the image's first
pixel - one space under two names.

The camera crosses World to screen with a single scale: `256 * 2^zoom` pixels
per world unit. That is the slippy-map convention — it makes one tile exactly
256 pixels wide when the camera's zoom matches the tile's own z. Note that the
camera's zoom is continuous while a tile's z is an integer; they are two
numbers, and collapsing them into one is a mistake that hides until fractional
zoom arrives.

Those pixels are framebuffer pixels, not logical points. The same 800x600 window
reports 800x600 on a 1x display and 1600x1200 on a Retina one, so the map is
drawn at the display's true resolution — and covers half as much of the screen
where the ratio is 2. Every pixel quantity here comes from
`glfwGetFramebufferSize`, never `glfwGetWindowSize`.

Two chains meet at the draw call:

    where the tile goes:  tile id + tile-local -> World -> screen -> NDC
    what colour it is:    image pixels -> texture -> sampled by uv

`LonLat -> World` is the projection in `src/mercator.cpp`, the way a geographic
position enters the first chain.

Note the `y grows` column: every space runs southward except NDC. **The y axis
is flipped exactly once in the whole pipeline, in the screen-to-NDC step at the
end of `tileToNDC`.** A second flip anywhere - `stbi_set_flip_vertically_on_load`,
reversed texture coordinates - cancels the first and lands the map upside down.

Screen space in the table above is the renderer's own: pixels measured from the
window centre. Input brings two more pixel conventions that are not it - GLFW
reports cursor positions from the window's top-left with y down, and `glViewport`
measures from the bottom-left with y up. Keeping the three apart is the next
place a sign error can hide.

## Tiles

Tile images are not checked in — they are not ours to redistribute. Fetch the
four the program currently expects:

```bash
for x in 0 1; do
  mkdir -p "tiles/1/$x"
  for y in 0 1; do
    curl -A "raster-map/0.1 (learning project)" \
         -o "tiles/1/$x/$y.png" "https://tile.openstreetmap.org/1/$x/$y.png"
  done
done
```

That is the whole world at zoom 1, quartered. The `-A` is not optional:
OpenStreetMap's tile policy requires a User-Agent that identifies the
application, and a request without one comes back as HTTP 200 carrying an
"access blocked" image rather than an error — so it fails silently, and the map
renders the notice.

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
