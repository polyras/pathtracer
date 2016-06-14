#include "math.h"

quadratic_result SolveQuadratic(fp32 A, fp32 B, fp32 C) {
  quadratic_result Result;
  Result.SolutionExists = false;

  fp32 Discriminant = B * B - 4 * A * C;
  if(Discriminant > 0) {
    fp32 DiscriminantSquareRoot = SqrtFP32(Discriminant);

    fp32 Q;
    if(B < 0) {
      Q = -.5f * (B - DiscriminantSquareRoot);
    }
    else {
      Q = -.5f * (B + DiscriminantSquareRoot);
    }

    Result.SolutionExists = true;
    Result.Root1 = Q / A;
    Result.Root2 = C / Q;
  }

  return Result;
}
