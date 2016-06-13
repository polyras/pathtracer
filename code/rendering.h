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
  v3fp32 Vertices[3];
  color Albedo;
  bool Intersect(ray Ray, fp32 *Distance) const;
  v3fp32 CalcNormal() const;
};

struct scene {
  camera Camera;
  sun Sun;
  triangle Triangles[50];
  memsize TriangleCount;

  scene();
  void AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2, color Albedo);
};

struct resolution {
  v2ui16 Dimension;

  memsize CalcCount() {
    return Dimension.X * Dimension.Y;
  }
};

void InitRendering(resolution Resolution);
void Render(color *Buffer, scene const *Scene);
void TerminateRendering();
