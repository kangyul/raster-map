#include <cmath>

#include "camera.hpp"

NDCRect tileToNDC(TileId tile, Camera camera, int w, int h) {
  int n = 1 << tile.z;
  double s = pixelsPerWorldUnit(camera.zoom);

  float scaleX =  (2.0 * s) / (static_cast<double>(n) * w);
  float scaleY = -(2.0 * s) / (static_cast<double>(n) * h); // The one y flip in the pipeline: tile y grows south, NDC y grows north.
  float offsetX = (2.0 * tile.x * s) / (static_cast<double>(n) * w) - (2.0 * s * camera.center.x) / w;
  float offsetY = (2.0 * s * camera.center.y) / h - (2.0 * tile.y * s) / (static_cast<double>(n) * h);
  return {.offsetX = offsetX, .offsetY = offsetY, .scaleX = scaleX, .scaleY = scaleY};
}

double pixelsPerWorldUnit(double zoom) {
  return 256.0 * std::exp2(zoom);
}

