#pragma once

// atan(sinh(pi)) in degrees — the latitude where Mercator y hits pi.
constexpr double kMaxLatitudeDeg = 85.0511287798066; 

struct LonLat {
  double lon, lat; // degrees
};

struct WorldPos {
  double x, y; // range: [0, 1]
};

struct TileId {
  int z, x, y;
};

WorldPos project(LonLat);
LonLat unproject(WorldPos);
TileId tileAt(WorldPos, int zoom);