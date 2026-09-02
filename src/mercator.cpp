#include "mercator.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

double degToRad(double deg) { return deg * kPi / 180.0; }
double radToDeg(double rad) { return rad * 180.0 / kPi; }

} // namespace

WorldPos project(LonLat lonLat) {
  const double x = (lonLat.lon + 180.0) / 360.0; // not wrapped yet

  // tan() near the poles is huge but finite, so an unclamped latitude fails silently.
  const double lat = std::clamp(lonLat.lat, -kMaxLatitudeDeg, kMaxLatitudeDeg);
  const double mercatorY = std::asinh(std::tan(degToRad(lat)));

  // -pi..+pi with north positive, to [0, 1] with north at zero.
  const double y = (1.0 - mercatorY / kPi) / 2.0;

  return {.x = x, .y = y};
}

LonLat unproject(WorldPos world) {
  const double lon = world.x * 360.0 - 180.0;
  const double lat = radToDeg(std::atan(std::sinh(kPi * (1.0 - world.y * 2.0))));

  return {.lon = lon, .lat = lat};
}

TileId tileAt(WorldPos world, int zoom) {
  const int n = 1 << zoom;
  const int x = std::clamp(static_cast<int>(std::floor(world.x * n)), 0, n-1);
  const int y = std::clamp(static_cast<int>(std::floor(world.y * n)), 0, n-1);

  return {.z = zoom, .x = x, .y = y};
}