#pragma once

#include "mercator.hpp"

#include <vector>

struct Camera {
  WorldPos center;
  double zoom;
};

struct NDCRect {
  float offsetX, offsetY, scaleX, scaleY;
};

struct WorldRect {
  WorldPos min, max;
};

NDCRect tileToNDC(TileId tile, Camera camera, int w, int h);
double pixelsPerWorldUnit(double zoom);

WorldRect visibleWorldRect(Camera camera, int w, int h);
std::vector<TileId> visibleTiles(Camera camera, int w, int h);
