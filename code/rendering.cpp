#include "rendering.h"

void Draw(frame_buffer *Buffer, scene *Scene) {
  v2ui16 ResDim = Buffer->Resolution.Dimension;
  ui32 YOffset;
  for(ui16 Y=0; Y<ResDim.Y; ++Y) {
    YOffset = Y * ResDim.X;
    for(ui16 X=0; X<ResDim.X; ++X) {
      ui8 *Pixel = Buffer->Pixels + (YOffset + X) * 3;
      Pixel[0] = 255;
      Pixel[1] = Y/5;
      Pixel[2] = 0;
    }
  }
}
