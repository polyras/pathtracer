#pragma once

#include <math.h>
#include "lib/def.h"

inline fp32 TanFP32(fp32 Angle) {
  return tan(Angle);
}

inline fp32 SqrtFP32(fp32 N) {
  return sqrt(N);
}

struct v2ui16 {
  ui16 X;
  ui16 Y;
};

struct v3fp32;
v3fp32 operator/(v3fp32 V, fp32 S);

struct v3fp32 {
  fp32 X;
  fp32 Y;
  fp32 Z;

  v3fp32() { }

  v3fp32(fp32 X, fp32 Y, fp32 Z) {
    Set(X, Y, Z);
  }

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

  fp32 CalcSquaredLength() const {
    return (
      X * X +
      Y * Y +
      Z * Z
    );
  }

  fp32 CalcLength() const {
    return SqrtFP32(CalcSquaredLength());
  }

  static v3fp32 Normalize(v3fp32 V) {
    return V / V.CalcLength();
  }
};

inline v3fp32 operator+(v3fp32 A, v3fp32 B) {
  v3fp32 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  Result.Z = A.Z + B.Z;
  return Result;
}

inline v3fp32& operator+=(v3fp32 &A, v3fp32 B) {
  A = A + B;
  return A;
}

inline v3fp32 operator-(v3fp32 A, v3fp32 B) {
  v3fp32 Result;
  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  Result.Z = A.Z - B.Z;
  return Result;
}

inline v3fp32& operator-=(v3fp32 &A, v3fp32 B) {
  A = A - B;
  return A;
}

inline v3fp32 operator*(v3fp32 V, fp32 S) {
  v3fp32 Result;
  Result.X = V.X * S;
  Result.Y = V.Y * S;
  Result.Z = V.Z * S;
  return Result;
}

inline v3fp32& operator*=(v3fp32 &V, fp32 S) {
  V = V * S;
  return V;
}

inline v3fp32 operator/(v3fp32 V, fp32 S) {
  v3fp32 Result;
  Result.X = V.X / S;
  Result.Y = V.Y / S;
  Result.Z = V.Z / S;
  return Result;
}

inline v3fp32& operator/=(v3fp32 &V, fp32 S) {
  V = V / S;
  return V;
}

inline fp32 DegToRad(fp32 Angle) {
  return (Angle/360) * (M_PI*2);
}
