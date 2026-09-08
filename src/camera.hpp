#pragma once

#include "mercator.hpp"

struct Camera {
  WorldPos center;
  double zoom;
};

struct NDCRect {
  float offsetX, offsetY, scaleX, scaleY;
};

NDCRect tileToNDC(TileId tile, Camera camera, int w, int h);
double pixelsPerWorldUnit(double zoom);

