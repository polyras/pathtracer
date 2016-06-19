#include "primitives.h"

static const fp32 Epsilon = 0.0001f;

bool triangle::Intersect(ray Ray, fp32 *Distance) const {
  v3fp32 VertexB = Vertices[1] - Vertices[0];
  v3fp32 VertexC = Vertices[2] - Vertices[0];
  v3fp32 RayDirectionCrossVertexC = v3fp32::Cross(Ray.Direction, VertexC);
  fp32 KDet = v3fp32::Dot(RayDirectionCrossVertexC, VertexB);

  if(KDet > -Epsilon) {
    return false;
  }
  fp32 KDetInv = 1.0f / KDet;
  v3fp32 RayOrigin = Ray.Origin - Vertices[0];

  fp32 BaryU = KDetInv * v3fp32::Dot(RayDirectionCrossVertexC, RayOrigin);
  if(BaryU > 1 || BaryU < 0) {
    return false;
  }

  v3fp32 RayOriginCrossVertexB = v3fp32::Cross(RayOrigin, VertexB);
  fp32 BaryV = KDetInv * v3fp32::Dot(Ray.Direction, RayOriginCrossVertexB);
  if(BaryV > 1 || BaryV < 0) {
    return false;
  }

  if(BaryU + BaryV > 1) {
    return false;
  }

  fp32 T = KDetInv * v3fp32::Dot(RayOriginCrossVertexB, VertexC);
  if(T < Epsilon) {
    return false;
  }

  *Distance = T;

  return true;
}

bool sphere::Intersect(ray Ray, fp32 *Distance) const {
  const v3fp32 LocalRayOrigin = Ray.Origin - Pos;
  const fp32 A = Ray.Direction.X * Ray.Direction.X + Ray.Direction.Y * Ray.Direction.Y + Ray.Direction.Z * Ray.Direction.Z;
  const fp32 B = 2 * (LocalRayOrigin.X * Ray.Direction.X + LocalRayOrigin.Y * Ray.Direction.Y + LocalRayOrigin.Z * Ray.Direction.Z);
  const fp32 C = LocalRayOrigin.X * LocalRayOrigin.X + LocalRayOrigin.Y * LocalRayOrigin.Y + LocalRayOrigin.Z * LocalRayOrigin.Z - Radius * Radius;
  quadratic_result QuadraticResult = SolveQuadratic(A, B, C);

  if(!QuadraticResult.SolutionExists) {
    return false;
  }

  if(QuadraticResult.Root1 > Epsilon) {
    if(QuadraticResult.Root2 > Epsilon) {
      *Distance = MinFP32(QuadraticResult.Root1, QuadraticResult.Root2);
    }
    else {
      *Distance = QuadraticResult.Root1;
    }
    return true;
  }
  else if(QuadraticResult.Root2 > Epsilon) {
    *Distance = QuadraticResult.Root2;
    return true;
  }
  else {
    return false;
  }
}

v3fp32 sphere::CalcNormal(v3fp32 SurfacePoint) const {
  v3fp32 Dif = SurfacePoint - Pos;
  Dif.Normalize();
  return Dif;
}

v3fp32 triangle::CalcNormal() const {
  v3fp32 Edge0 = Vertices[1] - Vertices[0];
  v3fp32 Edge1 = Vertices[2] - Vertices[0];
  v3fp32 Normal = v3fp32::Cross(Edge1, Edge0);
  Normal.Normalize();
  return Normal;
}
