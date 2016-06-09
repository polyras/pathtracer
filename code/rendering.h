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
};

struct sun {
  v3fp32 Direction;
};

struct ray;

struct triangle {
  v3fp32 Vertices[3];
  bool Intersect(ray Ray, fp32 *Distance) const;
  v3fp32 CalcNormal() const;
};

struct scene {
  camera Camera;
  sun Sun;
  triangle Triangles[50];
  memsize TriangleCount;

  scene();
  void AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2);
};

struct resolution {
  v2ui16 Dimension;
};

struct frame_buffer {
  resolution Resolution;
  color *Pixels;
};

void Draw(frame_buffer *Buffer, scene *Scene);
