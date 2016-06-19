#pragma once

#include "lib/math.h"
#include "primitives.h"

struct camera {
  v3fp32 Position;
  v3fp32 Direction;
  v3fp32 Right;
  fp32 FOV;
};

struct sun {
  v3fp32 Position;
  fp32 Irradiance;
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
