#include "lib/math.h"

struct ray {
  v3fp32 Origin;
  v3fp32 Direction;
};

struct color {
  ui8 R;
  ui8 G;
  ui8 B;

  color() { }

  color(ui8 R, ui8 G, ui8 B) : R(R), G(G), B(B) { }
};

struct triangle {
  memsize ID;
  v3fp32 Vertices[3];
  color Albedo;
  bool Intersect(ray Ray, fp32 *Distance) const;
  v3fp32 CalcNormal() const;
};

struct sphere {
  memsize ID;
  v3fp32 Pos;
  fp32 Radius;
  v3fp32 Intensity;
  color Albedo;
  bool Intersect(ray Ray, fp32 *Distance) const;
  v3fp32 CalcNormal(v3fp32 SurfacePoint) const;
};
