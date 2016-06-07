#include "lib/math.h"

struct resolution {
  v2ui16 Dimension;

};

struct frame_buffer {
  resolution Resolution;
  ui8 *Pixels;
};

void Draw(frame_buffer *Buffer);
