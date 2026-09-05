---
name: pipeline
description: Orientation for the raster-map codebase - what each file does, how a tile becomes pixels (GLFW -> VAO/VBO -> texture -> tile-to-NDC transform -> draw call), the coordinate spaces and the invariants that hold between them, and the silent-failure checklist for a black or upside-down window. Load before answering "where does X happen", "why is my quad black/flipped/stretched", or before adding a camera, viewport tile selection, a cache, or HTTP fetching.
---

# raster-map: structure and pipeline

A raster tile map renderer built from scratch in C++20 and OpenGL 3.3 core. It is
a learning project: the point is to understand a slippy map by building one.

Read this for orientation, then read the code. Where this file and the code
disagree, the code is right - and fix this file.

## Layout

| Path | Role |
|---|---|
| `CMakeLists.txt` | FetchContent pulls GLFW 3.5.1 + stb; `find_package(OpenGL)`. Exports `compile_commands.json` for clangd. |
| `src/main.cpp` | Window, geometry, texture upload, tile->NDC math, frame loop, both GLSL sources as string literals. Everything not yet factored out. |
| `src/mercator.{hpp,cpp}` | Pure math, zero GL: `project`, `unproject`, `tileAt`. |
| `src/shader.{hpp,cpp}` | RAII wrapper: compile, link, report the info log, delete. Non-copyable. |
| `src/stb_image_impl.cpp` | The one TU that defines `STB_IMAGE_IMPLEMENTATION`. |
| `tiles/z/x/y.png` | Gitignored - not ours to redistribute. Must be fetched. |

`scratch` is a second, guarded executable: it exists only when `src/scratch.cpp`
does, and that file is gitignored. It is the sandbox for breaking things -
`cp src/main.cpp src/scratch.cpp` and experiment without touching the real target.

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
9. Clear, `use()`, bind the VAO once, then per tile: `tileToNDC` ->
   `glUniform4f` -> `glBindTexture` -> `glDrawElements(GL_TRIANGLES, 6, ...)`.
   Swap buffers, poll events.
10. Texture unit 0 is active by default and a `sampler2D` defaults to unit 0, so
    with a single texture there is no `glActiveTexture` / `glUniform1i`. Adding
    a second sampler changes that.

The transform is one line of GLSL:

```glsl
gl_Position = vec4(aPos.xy * uTileRect.zw + uTileRect.xy, 0.0, 1.0);
```

fed by `tileToNDC`, with `n = 1 << z`:

```
scale  = ( 2/n, -2/n )
offset = ( 2x/n - 1, 1 - 2y/n )
```

Sanity check z=1, x=y=0: local `(0,0)` -> `(-1, 1)` (top-left of screen), local
`(1,1)` -> `(0,0)` (centre). The NW tile fills the upper-left quadrant.

That affine map **is** the camera, currently hardcoded. Pan and zoom will change
what feeds `uTileRect`, not the shader.

## Coordinate spaces

| Space | Range | y grows | Lives in |
|---|---|---|---|
| Geographic | lon +/-180 deg, lat +/-85.0511 deg | north | `LonLat` |
| World | `[0,1]` square, origin north-west | south | `WorldPos` |
| Tile id | integers `0 .. 2^z - 1` | south | `TileId` |
| Tile-local | `[0,1]` within one tile | south | quad vertex positions |
| Texture | `[0,1]` uv | south | quad vertex uvs |
| Image | 256x256 pixels, row 0 is north | south | `stbi_load` buffer |
| NDC | `[-1,1]`, origin centre | **north** | `gl_Position` |

Two chains meet at the draw call:

```
where the tile goes:  LonLat -> World -> tile id + tile-local -> NDC
what colour it is:    image pixels -> texture -> sampled by uv
```

## Invariants - break one and the symptom is silent

- **The y axis flips exactly once**, in `tileToNDC`'s negative `scaleY`. A second
  flip anywhere - `stbi_set_flip_vertically_on_load`, reversed uvs - cancels it
  and the map lands upside down. Never "fix" an inverted map by adding a flip;
  find the one that shouldn't be there.
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

1. **Black window, nothing drawn** - did the shader compile and link? The
   `Shader` class prints the info log; check stdout/stderr first.
2. **Geometry there, black or white** - texture. Did `stbi_load` return null
   (path wrong, or run from the wrong directory)? Is a mipmapping min filter set
   with no mipmaps? Is the texture bound at draw time?
3. **Map upside down** - count the flips (see Invariants). Exactly one.
4. **Map stretched** - there is no `glViewport` call and no framebuffer-size
   callback. The square world is drawn into a non-square window and resizing does
   not update the viewport. Known gap, not a bug to hunt.
5. **Tiles in the wrong quadrant** - hand-evaluate `tileToNDC` for that `z/x/y`
   and compare against the sanity check above.

## State of the work

Draws a hardcoded 2x2 grid of zoom-1 tiles. Working, and honest about its gaps:

- `mercator.cpp` is **correct but unused** - the renderer never calls `project`
  or `tileAt`. Nothing computes which tiles a viewport needs.
- No camera. The view is fixed.
- No aspect-ratio handling (see above).
- No tile cache, no eviction, no HTTP fetching.

The missing join, and the next real step:

```
viewport bounds -> LonLat -> WorldPos -> tileAt -> {TileId} -> load + draw
```

Wiring that makes `mercator` live code and replaces the hardcoded loop in `run()`.

Check `README.md`'s Planned scope for the current checklist, and prefer the code
over both documents when they disagree.

## Working style here

This is a learning repo, not a delivery repo. Prefer to explain the mechanism and
hand over the step rather than silently producing finished code. The useful
exercise format is predict-then-break: change one line, write the prediction
down, then run - a wrong prediction is the point. Boilerplate and API lookups
carry no such value; just supply those.
