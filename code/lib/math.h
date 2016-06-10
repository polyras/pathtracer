#pragma once

#include <math.h>
#include "lib/def.h"

inline fp32 TanFP32(fp32 Angle) {
  return tanf(Angle);
}

inline fp32 SinFP32(fp32 Angle) {
  return sinf(Angle);
}

inline fp32 CosFP32(fp32 Angle) {
  return cosf(Angle);
}

inline fp32 SqrtFP32(fp32 N) {
  return sqrt(N);
}

struct v2ui16 {
  ui16 X;
  ui16 Y;

  void Set(fp32 NewX, fp32 NewY) {
    X = NewX;
    Y = NewY;
  }
};

struct v3fp32;
v3fp32 operator/(v3fp32 V, fp32 S);

struct v3fp32 {
  fp32 X;
  fp32 Y;
  fp32 Z;

  v3fp32() { }

  explicit v3fp32(fp32 R) : X(R), Y(R), Z(R) { }

  explicit v3fp32(fp32 X, fp32 Y, fp32 Z) : X(X), Y(Y), Z(Z) { }

  void Clear() {
    X = 0;
    Y = 0;
    Z = 0;
  }

  void Normalize() {
    *this = v3fp32::Normalize(*this);
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

  static fp32 Dot(v3fp32 A, v3fp32 B) {
    fp32 Result = (
      A.X * B.X +
      A.Y * B.Y +
      A.Z * B.Z
    );
    return Result;
  }

  static v3fp32 Hadamard(v3fp32 A, v3fp32 B) {
    v3fp32 Result;
    Result.X = A.X * B.X;
    Result.Y = A.Y * B.Y;
    Result.Z = A.Z * B.Z;
    return Result;
  }

  static v3fp32 Cross(v3fp32 A, v3fp32 B) {
    v3fp32 Result;
    Result.X = A.Y*B.Z - A.Z*B.Y;
    Result.Y = A.Z*B.X - A.X*B.Z;
    Result.Z = A.X*B.Y - A.Y*B.X;
    return Result;
  }
};

inline v3fp32 operator-(v3fp32 const &V) {
  v3fp32 Result(-V.X, -V.Y, -V.Z);
  return Result;
}

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

struct m33fp32 {
  v3fp32 Col1;
  v3fp32 Col2;
  v3fp32 Col3;
};

inline v3fp32 operator*(m33fp32 M, v3fp32 V) {
  v3fp32 Result;
  Result.X = M.Col1.X * V.X + M.Col2.X * V.Y + M.Col3.X * V.Z;
  Result.Y = M.Col1.Y * V.X + M.Col2.Y * V.Y + M.Col3.Y * V.Z;
  Result.Z = M.Col1.Z * V.X + M.Col2.Z * V.Y + M.Col3.Z * V.Z;
  return Result;
}

inline fp32 DegToRad(fp32 Angle) {
  return (Angle/360) * (M_PI*2);
}

inline fp32 MinFP32(fp32 A, fp32 B) {
  return A < B ? A : B;
}

inline memsize MinMemsize(memsize A, memsize B) {
  return A < B ? A : B;
}

inline memsize RoundFP32(fp32 R) {
  return static_cast<memsize>(floor(R + 0.5f));
}
