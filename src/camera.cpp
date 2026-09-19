#include <cmath>

#include "camera.hpp"
#include "mercator.hpp"

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

WorldRect visibleWorldRect(Camera camera, int w, int h) {
  WorldRect r;

  double s = pixelsPerWorldUnit(camera.zoom);

  double minX = camera.center.x - (w / 2.0) / s;
  double minY = camera.center.y - (h / 2.0) / s;
  r.min = {.x = minX, .y = minY};

  double maxX = camera.center.x + (w / 2.0) / s;
  double maxY = camera.center.y + (h / 2.0) / s;
  r.max = {.x = maxX, .y = maxY};

  return r;
}

std::vector<TileId> visibleTiles(Camera camera, int w, int h) {
  const WorldRect r = visibleWorldRect(camera, w, h);
  int z = static_cast<int>(std::floor(camera.zoom));

  TileId minTile = tileAt(r.min, z);
  TileId maxTile = tileAt(r.max, z);

  std::vector<TileId> tiles;

  for(int x = minTile.x; x <= maxTile.x; ++x) {
    for(int y = minTile.y; y <= maxTile.y; ++y) {
      tiles.push_back({.z = z, .x = x, .y = y});
    }
  }

  return tiles;
}
