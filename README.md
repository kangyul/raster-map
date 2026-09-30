# raster-map

A raster tile map renderer, written from scratch in C++ and OpenGL.

This is a learning project. The goal is to understand how a slippy map actually
works — Web Mercator projection, a tile pyramid, texture upload, and the camera
math that makes panning and zooming feel right — by building one rather than
reading about it.

## Status

Draws the tiles the viewport needs, through a 2D camera. The camera is a world
position to center on plus a continuous zoom level, and `tileToNDC` places each
tile from its z/x/y address by way of that camera, so the map holds its square
shape in a window of any proportion.

Dragging pans the map. The scroll wheel or trackpad zooms toward the cursor.
The point under the cursor stays where it is while the zoom changes.
The zoom level doesn't go below 0.

Each frame, `visibleTiles` inverts the camera on the window's edges to get the
World rectangle on screen, picks the tile level `floor(zoom)` (clamped to the
z0–z5 pyramid on disk), and converts the rectangle's corners into a range of
tile ids with `tileAt` from `src/mercator.cpp`. A tile's PNG is loaded the first
time the tile comes into view. Past zoom 5 the z5 tiles are simply drawn larger.

Loaded textures live in a least-recently-used cache. Every visible tile is
stamped with the current frame number, and after drawing, the oldest entries are
deleted until the cache holds no more than `max(128, 2 × visible tiles)`. A
tile whose file failed to load stays cached as a blank entry, so a missing file
is not re-read every frame.

## Planned scope

- [x] Window and GL context (GLFW)
- [x] Render a single textured quad
- [x] Load PNG tiles from disk
- [x] Web Mercator projection and tile addressing (z/x/y)
- [x] Pan and zoom with a 2D camera
- [x] Load only the tiles the viewport needs
- [x] Tile cache with eviction
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

Panning runs that scale backwards. Write `screen = (p - centre) * s` for the
frame before a mouse move and again for the frame after, holding `p` — the world
point under the cursor — the same in both, and `p` drops out: `centre -= delta /
s`. That cancellation is why dragging needs no unprojection and no hit test. It
is also the whole of the input code: the camera moves, and neither the shader
nor `tileToNDC` knows anything happened.

Zooming holds the other variable fixed. The cursor stays put while `s` changes,
so write the same equation with `m` - the cursor's offset from the window
centre, in framebuffer pixels - at the scale before and after: `m = (p -
centre) * s` and `m = (p - centre') * s'`. This time `p` does not drop out. It
has to be recovered first, `p = centre + m / s`, which makes zoom the first
place the renderer runs screen to World backwards - and running that inverse on
the window's corners is how `visibleTiles` finds which tiles the viewport
needs. Substituting gives `centre' = centre + m / s - m / s'`, or read the other
way, `centre' = p - m / s'`: the new centre sits `m / s'` from the pinned point,
so each doubling of the scale halves the distance between them.

Two checks catch the likely mistakes: `m = 0` must leave the centre alone, and
so must `s' = s`. The second matters more than it looks, because the correction
runs every frame whether or not anything scrolled - a flipped sign passes the
first check and drifts the map with no input at all. For the same reason the
zoom floor of `0` is applied before `s'` is computed: clamp afterwards and
scrolling at the floor slides the map sideways while the zoom holds still.

Those pixels are framebuffer pixels, not logical points. The same 800x600 window
reports 800x600 on a 1x display and 1600x1200 on a Retina one, so the map is
drawn at the display's true resolution — and covers half as much of the screen
where the ratio is 2. The two sizes have one job each: everything that renders
takes framebuffer pixels from `glfwGetFramebufferSize` — `glViewport`, and the
`w` and `h` in the transform — and `glfwGetWindowSize` is read for exactly one
purpose, the ratio between the two units.

Two chains meet at the draw call:

    where the tile goes:  tile id + tile-local -> World -> screen -> NDC
    what colour it is:    image pixels -> texture -> sampled by uv

`LonLat -> World` is the projection in `src/mercator.cpp`, the way a geographic
position enters the first chain.

Note the `y grows` column: every space runs southward except NDC. **The y axis
is flipped exactly once in the whole pipeline, in the screen-to-NDC step at the
end of `tileToNDC`.** A second flip does not simply cancel the first - where it
is added decides how it fails.

A texture-side flip - `stbi_set_flip_vertically_on_load`, reversed texture
coordinates - leaves the quads where they are and only changes which texel a uv
samples, so every tile mirrors inside its own rectangle. The tiles keep their
north-to-south order while their contents do not: the vertical seams still line
up and every horizontal one breaks. Back when a single quad covered the whole
map this was indistinguishable from a plain upside-down map, which is where the
simpler story came from.

A geometry-side flip - the sign of `scaleY` - both mirrors and moves. `offset`
pins the `v=0` edge and `scale` decides which way the tile grows from it, so
reversing the sign slides each tile a full tile-height up the screen as well as
mirroring it.

Screen space in the table above is the renderer's own: pixels measured from the
window centre. Two other pixel conventions are live alongside it and are not it -
`glfwGetCursorPos` reports from the window's top-left in **logical points** with
y down, and `glViewport` measures from the bottom-left in framebuffer pixels with
y up. Cursor deltas therefore get scaled by the framebuffer-to-window ratio
before they are divided by `s`; without that step panning tracks the cursor on a
1x display and runs at half speed on a 2x one. Zooming needs the cursor's
position rather than its movement, so it also moves the origin: subtract half
the window size, in logical points, and then scale. Leave the scaling out there
and a 2x display pins the point halfway between the window centre and the
cursor instead of the one under it. Cursor y needs no flip, though,
because it runs southward like the renderer's screen space - that agreement is
why the y axis still flips exactly once.

## Tiles

Tile images are not checked in — they are not ours to redistribute. The program
reads them from `tiles/{z}/{x}/{y}.png` and expects the full pyramid from z0 to
z5 (1365 tiles). `scripts/fetch_tiles.sh` downloads it from MapTiler, which
needs a free API key:

```bash
MAPTILER_KEY=your-key ./scripts/fetch_tiles.sh      # z0..z5
MAPTILER_KEY=your-key ./scripts/fetch_tiles.sh 3    # stop at z3
```

The script skips files it already has, so it is safe to re-run after an
interruption, and it reports any tiles that failed. A missing tile is not fatal:
the renderer leaves that square empty. The renderer never looks past z5, so
fetching deeper levels only takes up disk space.

Tiles are the MapTiler Aquarelle style: © MapTiler © OpenStreetMap contributors.

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
