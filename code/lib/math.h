#pragma once

#include <math.h>
#include "lib/def.h"

struct v2ui16 {
  ui16 X;
  ui16 Y;
};

struct v3fp32 {
  fp32 X;
  fp32 Y;
  fp32 Z;

  void Clear() {
    X = 0;
    Y = 0;
    Z = 0;
  }

  void Set(fp32 NewX, fp32 NewY, fp32 NewZ) {
    X = NewX;
    Y = NewY;
    Z = NewZ;
  }
};

inline fp32 DegToRad(fp32 Angle) {
  return (Angle/360) * (M_PI*2);
}
