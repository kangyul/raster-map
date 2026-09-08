---
name: pipeline
description: Orientation for the raster-map codebase - what each file does, how a tile becomes pixels (GLFW -> VAO/VBO -> texture -> camera -> tile-to-NDC transform -> draw call), the coordinate spaces and the invariants that hold between them, and the silent-failure checklist for a black, flipped, displaced or stretched window. Load before answering "where does X happen", "why is my quad black/flipped/stretched", or before adding pan/zoom input, viewport tile selection, a cache, or HTTP fetching.
---

# raster-map: structure and pipeline

A raster tile map renderer built from scratch in C++20 and OpenGL 3.3 core. It is
a learning project: the point is to understand a slippy map by building one.

Read this for orientation, then read the code. Where this file and the code
disagree, the code is right - and fix this file.

## Layout

| Path | Role |
|---|---|
| `CMakeLists.txt` | FetchContent pulls GLFW 3.5.1 + stb; `find_package(OpenGL)`. Shared sources live in a `map-core` OBJECT library whose include path, language level, definitions, warnings and GL links are `PUBLIC`, so both executables inherit them by linking. |
| `src/main.cpp` | Window, geometry, texture upload, `Camera`, tile->NDC math, frame loop, both GLSL sources as string literals. Everything not yet factored out. |
| `src/mercator.{hpp,cpp}` | Pure math, zero GL: `project`, `unproject`, `tileAt`. |
| `src/shader.{hpp,cpp}` | RAII wrapper: compile, link, report the info log, delete. Non-copyable. |
| `src/stb_image_impl.cpp` | The one TU that defines `STB_IMAGE_IMPLEMENTATION`. |
| `tiles/z/x/y.png` | Gitignored - not ours to redistribute. Must be fetched. |

`scratch` is a second, guarded executable: it exists only when `src/scratch.cpp`
does, and that file is gitignored. It is the sandbox for breaking things -
`cp src/main.cpp src/scratch.cpp` and experiment without touching the real
target. Rename its window title when you do, or two identical windows will be
telling you two different stories.

`CMAKE_EXPORT_COMPILE_COMMANDS` is set at the top of `CMakeLists.txt` and must
stay there: the variable is read when each target is *created*, so a target
declared above the `set()` is silently left out of `compile_commands.json` and
clangd loses that file's flags.

## Build and run

```bash
cmake -B build && cmake --build build
./build/raster-map          # from the repo root, NOT from build/
```

Tile paths are relative to the working directory. Running from `build/` fails
every texture load.

## The pipeline, data to pixels

Startup - `main()`:
1. GLFW hints for GL 3.3 **core + forward-compatible**. macOS refuses a 3.3
   context without both.
2. Create window, then `glfwMakeContextCurrent` before any `gl*` call.
3. No GLAD/GLEW. `<OpenGL/gl3.h>` supplies core symbols straight from the system
   framework. This is why there is no loader dependency, and why `main.cpp` and
   `shader.cpp` are **macOS-only as written** - porting means adding a loader.
   Never also include `<OpenGL/gl.h>`: together they re-declare the removed
   fixed-function calls, so using one compiles and then fails silently at
   runtime instead of erroring at build time.

Setup - `run()`:
4. `Shader(vs, fs)` compiles GLSL **at runtime**. A shader typo is a runtime
   failure with a black window, not a build error; the info-log plumbing in
   `shader.cpp` is what makes it visible.
5. Geometry: 4 vertices of 5 floats (`x y z u v`) plus a 6-index EBO. Positions
   are in **tile-local `[0,1]`, y down**, so `uv` and `xy` hold identical
   numbers - deliberately (see Invariants).
6. Bind the VAO *first*, then fill VBO/EBO, then `glVertexAttribPointer`
   (stride 20 B, offsets 0 and 12) and enable. The VAO records the attribute
   format, which VBO each attribute reads from, and the EBO binding - which is
   why one `glBindVertexArray` per frame restores all of it.
7. Cache the `uTileRect` uniform location once.
8. `loadTexture` per tile: `stbi_load(..., 4)` forces RGBA and does not flip
   rows; `CLAMP_TO_EDGE` on S and T; `NEAREST` min and mag; `glTexImage2D`
   level 0; free the pixels.

Per frame:
9. `glfwGetFramebufferSize` -> `glViewport`, and the same width and height feed
   `tileToNDC`. Both must come from the framebuffer, never `glfwGetWindowSize`:
   on a 2x display those differ by a factor of two.
10. Clear, `use()`, bind the VAO once, then per tile: `tileToNDC` ->
    `glUniform4f` -> `glBindTexture` -> `glDrawElements(GL_TRIANGLES, 6, ...)`.
    Swap buffers, poll events.
11. Texture unit 0 is active by default and a `sampler2D` defaults to unit 0, so
    with a single texture there is no `glActiveTexture` / `glUniform1i`. Adding
    a second sampler changes that.

The transform is one line of GLSL:

```glsl
gl_Position = vec4(aPos.xy * uTileRect.zw + uTileRect.xy, 0.0, 1.0);
```

fed by `tileToNDC`, which composes three affine steps:

```
tile-local -> world:      (t + q) / n          n = 1 << tile.z
world -> screen pixels:   (p - center) * s     s = 256 * 2^camera.zoom
screen -> NDC:            x / (w/2),  -y / (h/2)
```

Screen pixels here are measured from the **window centre with y down**, which is
why the composition needs no flip until the last step. Collapsed and grouped by
the tile-local coordinate:

```
scale  = (  2s/(n*w),              -2s/(n*h)              )
offset = ( (2s/w) * (x/n - cx),    (2s/h) * (cy - y/n)    )
```

`scale` carries no `center` and no tile index: a tile's size depends on neither
where the camera looks nor which tile it is. `offset` is where the tile's
`(u,v) = (0,0)` corner - its north-west one - lands in NDC.

**`n` and `s` are two different powers of two.** `n = 1 << tile.z` is an integer
tile count; `s` uses `std::exp2(camera.zoom)` because camera zoom is a
continuous `double`. Collapsing them works only while the camera sits at an
integer zoom equal to the tiles' own z, and breaks the moment scroll zoom
arrives.

Sanity check - camera `{0.5, 0.5}`, zoom 1, framebuffer 800x600, tile z=1:

```
s = 512,  n = 2
scale  = ( 0.64, -0.8533 )      for every tile
offset = ( -0.64, +0.8533 )     tile (0,0)
         (  0,    +0.8533 )     tile (1,0)
         ( -0.64,  0      )     tile (0,1)
         (  0,     0      )     tile (1,1)
```

The world lands as a 512x512 px square centred in the window: 144 px of
background either side, 44 px above and below. All four offsets share only two
distinct values per axis because the camera sits exactly on the corner where the
four tiles meet.

## Coordinate spaces

| Space | Range | y grows | Lives in |
|---|---|---|---|
| Geographic | lon +/-180 deg, lat +/-85.0511 deg | north | `LonLat` |
| World | `[0,1]` square, origin north-west | south | `WorldPos` |
| Tile id | integers `0 .. 2^z - 1` | south | `TileId` |
| Tile-local | `[0,1]` within one tile | south | quad vertex positions |
| Texture | `[0,1]` uv | south | quad vertex uvs |
| Image | 256x256 pixels, row 0 is north | south | `stbi_load` buffer |
| Screen | framebuffer pixels, origin at the window centre | south | inside `tileToNDC` |
| NDC | `[-1,1]`, origin centre | **north** | `gl_Position` |

Two chains meet at the draw call:

```
where the tile goes:  tile id + tile-local -> World -> screen -> NDC
what colour it is:    image pixels -> texture -> sampled by uv
```

`LonLat -> World` is `project` in `mercator.cpp`, the way a geographic position
enters the first chain. Input will add two more pixel conventions that are not
the Screen row above: `glfwGetCursorPos` measures from the window's top-left in
**logical points** with y down, and `glViewport` from the bottom-left in
framebuffer pixels with y up.

## Invariants - break one and the symptom is silent

- **The y axis flips exactly once**, in the screen->NDC step - the minus sign on
  `scaleY`. A second flip does **not** simply cancel the first. The two possible
  flips act at different stages and fail differently:

  | `scaleY` | texture flipped | tile contents | tile position |
  |---|---|---|---|
  | negative | no | correct | correct |
  | negative | **yes** | **mirrored** | correct |
  | **positive** | no | mirrored | **moved up one tile height** |
  | **positive** | **yes** | correct | **moved up one tile height** |

  Orientation is the XOR of the two flips; position depends on `scaleY` alone.
  A texture-side flip (`stbi_set_flip_vertically_on_load`, reversed uvs) cannot
  move anything - it only changes which texel a uv samples, so each tile mirrors
  inside its own rectangle: the vertical seams still line up and every
  horizontal one breaks. A geometry-side flip both mirrors and displaces,
  because `offset` pins the `v=0` edge and the sign decides which way the tile
  grows away from it. Back when a single quad covered the whole map a texture
  flip was indistinguishable from an upside-down map, which is where the older,
  simpler claim came from. Never "fix" an inverted map by adding a flip; find
  the one that should not be there.
- **`stbi_set_flip_vertically_on_load` is a global mode**, not an argument. It is
  read by every later `stbi_load`, so *when* it is called decides what it
  affects. Called after the first load, only the first tile comes out unflipped
  and the mosaic disagrees with itself. Call it once before any load, or not at
  all.
- **Texture coords equal tile-local positions** because a tile's top-left corner
  is the image's first pixel. One space under two names. If they ever diverge,
  something upstream flipped.
- **`NEAREST` min filter is load-bearing**: no mipmaps are generated, and a
  mipmapping min filter samples a level that does not exist -> black tiles.
- **`CLAMP_TO_EDGE` is load-bearing**: `REPEAT` wraps the far edge of the tile
  into the seam when interpolating.
- **The binary must run from the repo root** - tile paths are CWD-relative.

## When the window is wrong

GL fails silently. Work down this list rather than guessing:

1. **Is the window you are looking at built from the code you are reading?**
   Check first, every time: `ls -la build/<target> src/*.cpp`, and rebuild before
   interpreting anything. A stale binary explains a wrong window better than any
   theory about the maths, and `scratch` and `raster-map` open identically named
   windows unless the title has been changed. The same question applies to
   clangd's compilation database, to a screenshot pasted from a stale clipboard,
   and to anything else made earlier from a state that has since moved.
2. **Black window, nothing drawn** - did the shader compile and link? The
   `Shader` class prints the info log; check stdout/stderr first.
3. **Geometry there, black or white** - texture. Did `stbi_load` return null
   (path wrong, or run from the wrong directory)? Is a mipmapping min filter set
   with no mipmaps? Is the texture bound at draw time?
4. **Map mirrored, or displaced but correctly oriented** - count the flips
   against the table in Invariants. Which of the two symptoms appears tells you
   which flip is the wrong one.
5. **Map stretched** - `w` and `h` must both come from `glfwGetFramebufferSize`
   and reach `glViewport` and `tileToNDC` alike. Mixing in `glfwGetWindowSize`
   stretches by the display's scale factor on one monitor and not on another.
6. **Stretched only while dragging the window edge** - not a bug. macOS runs a
   nested event loop during a live resize, so `glfwPollEvents` does not return
   and the system scales the last frame until the drag ends.
7. **Tiles in the wrong quadrant** - hand-evaluate `tileToNDC` for that `z/x/y`
   and compare against the sanity check above.

## State of the work

Draws a hardcoded 2x2 grid of zoom-1 tiles through a 2D camera. Working, and
honest about its gaps:

- `mercator.cpp` is **correct but unused** - the renderer never calls `project`
  or `tileAt`. Nothing computes which tiles a viewport needs, and there are no
  tests.
- The camera exists - a `WorldPos` centre plus a continuous zoom - but **nothing
  changes it**. Its values are literals in `run()`; there are no input handlers.
- Aspect ratio is handled: the map keeps its square shape in any window.
- No tile cache, no eviction, no HTTP fetching.

Two joins are missing. Input, the next step:

```
drag pixels -> / s -> world delta -> camera.center
scroll -> camera.zoom
```

Cursor deltas arrive in logical points while `s` is in framebuffer pixels, so
that division needs the display's scale factor or it will be right on a 1x
monitor and half-speed on a 2x one. Then viewport tile selection:

```
viewport bounds -> LonLat -> WorldPos -> tileAt -> {TileId} -> load + draw
```

which makes `mercator` live code and replaces the hardcoded loop in `run()`.

Check `README.md`'s Planned scope for the current checklist, and prefer the code
over both documents when they disagree.

## Working style here

This is a learning repo, not a delivery repo. Prefer to explain the mechanism and
hand over the step rather than silently producing finished code. The useful
exercise format is predict-then-break: change one line, write the prediction
down, then run - a wrong prediction is the point. Boilerplate and API lookups
carry no such value; just supply those.

When a claim in this file or in `README.md` is contradicted by what the window
shows, the claim is what moves. The flip table above replaced a sentence that was
true of a single quad and was carried into the grid without being retested.
