#include "rendering.h"

#include <stdio.h>

struct ray {
  v3fp32 Origin;
  v3fp32 Direction;
};

void Draw(frame_buffer *Buffer, scene *Scene) {
  v3fp32 WorldPlaneCenter = Scene->Camera.Position + Scene->Camera.Direction;
  fp32 WorldPlaneWidth = TanFP32(Scene->Camera.FOV/2.0f)*2.0f;

  ui16 ScreenPlaneWidth = Buffer->Resolution.Dimension.X;
  ui16 ScreenPlaneHeight = Buffer->Resolution.Dimension.Y;
  ui16 HalfScreenPlaneWidth = ScreenPlaneWidth * 0.5f;
  ui16 HalfScreenPlaneHeight = ScreenPlaneHeight * 0.5f;

  fp32 ScreenToWorldPlaneRatio = static_cast<fp32>(WorldPlaneWidth) / (ScreenPlaneWidth);
  ray Ray = { .Origin = Scene->Camera.Position };
  v3fp32 Up(0, 1, 0);

  ui32 ScreenPixelYOffset;
  for(ui16 Y=0; Y<ScreenPlaneHeight; ++Y) {
    ScreenPixelYOffset = Y * ScreenPlaneWidth;
    fp32 ScreenRowCenterY = 0.5f + (static_cast<si16>(Y) - HalfScreenPlaneHeight);
    v3fp32 WorldRowCenter = WorldPlaneCenter + Up * (ScreenRowCenterY * ScreenToWorldPlaneRatio);
    for(ui16 X=0; X<ScreenPlaneWidth; ++X) {
      fp32 PixelColCenterX = 0.5f + (static_cast<si16>(X) - HalfScreenPlaneWidth);
      v3fp32 WorldPixelPosition = WorldRowCenter + Scene->Camera.Left * (PixelColCenterX * ScreenToWorldPlaneRatio);
      v3fp32 Difference = WorldPixelPosition - Scene->Camera.Position;
      Ray.Direction = v3fp32::Normalize(Difference);

      color *Pixel = Buffer->Pixels + ScreenPixelYOffset + X;

      fp32 P = static_cast<fp32>(Y)/ScreenPlaneHeight;
      Pixel->R = 255 * (P);
      Pixel->G = 255 * (P);
      Pixel->B = 255 * (P);
    }
  }
}
