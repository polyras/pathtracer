#include "lib/math.h"

struct camera {
  v3fp32 Position;
  v3fp32 Direction;
  v3fp32 Left;
  fp32 FOV;
};

struct color {
  ui8 R;
  ui8 G;
  ui8 B;
};

struct scene {
  camera Camera;
};

struct resolution {
  v2ui16 Dimension;
};

struct frame_buffer {
  resolution Resolution;
  color *Pixels;
};

void Draw(frame_buffer *Buffer, scene *Scene);
