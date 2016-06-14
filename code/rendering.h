#pragma once

#include "lib/math.h"

struct camera {
  v3fp32 Position;
  v3fp32 Direction;
  v3fp32 Right;
  fp32 FOV;
};

struct color {
  ui8 R;
  ui8 G;
  ui8 B;

  color() { }

  color(ui8 R, ui8 G, ui8 B) : R(R), G(G), B(B) { }
};

struct sun {
  v3fp32 Position;
  fp32 Irradiance;
};

struct ray;

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

struct scene {
  camera Camera;
  sun Sun;
  memsize NextObjectID = 0;
  triangle Triangles[50];
  memsize TriangleCount;
  sphere Spheres[10];
  memsize SphereCount;

  scene();
  void AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2, color Albedo);
  void AddSphere(v3fp32 Position, fp32 Radius, v3fp32 Intensity, color Albedo);
};

struct resolution {
  v2ui16 Dimension;

  memsize CalcCount() {
    return Dimension.X * Dimension.Y;
  }
};

memsize InitRendering(resolution Resolution);
void RenderTile(color *Buffer, scene const *Scene, memsize TileIndex);
void TerminateRendering();
