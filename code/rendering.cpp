#include "rendering.h"
#include "lib/assert.h"

struct ray {
  v3fp32 Origin;
  v3fp32 Direction;
};

struct trace_result {
  bool Hit;
  v3fp32 Position;
  v3fp32 Normal;
};

scene::scene() {
  TriangleCount = 0;
}

void scene::AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2) {
  triangle *T = Triangles + TriangleCount;
  T->Vertices[0] = V0;
  T->Vertices[1] = V1;
  T->Vertices[2] = V2;
  TriangleCount++;
}

bool triangle::Intersect(ray Ray, fp32 *Distance) const {
  v3fp32 VertexB = Vertices[1] - Vertices[0];
  v3fp32 VertexC = Vertices[2] - Vertices[0];
  v3fp32 RayDirectionCrossVertexC = v3fp32::Cross(Ray.Direction, VertexC);
  fp32 KDet = v3fp32::Dot(RayDirectionCrossVertexC, VertexB);
  if(KDet > -0.0001) {
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
  *Distance = T;

  return true;
}

v3fp32 triangle::CalcNormal() const {
  v3fp32 Edge0 = Vertices[1] - Vertices[0];
  v3fp32 Edge1 = Vertices[2] - Vertices[0];
  v3fp32 Normal = v3fp32::Cross(Edge1, Edge0);
  Normal.Normalize();
  return Normal;
}

static trace_result Trace(scene const *Scene, ray Ray) {
  memsize ClosestTriangleIndex = MEMSIZE_MAX;
  fp32 ShortestDistance = FP32_MAX;

  fp32 TestDistance;
  for(memsize I=0; I<Scene->TriangleCount; ++I) {
    const triangle *Triangle = Scene->Triangles + I;
    if(Triangle->Intersect(Ray, &TestDistance)) {
      if(TestDistance < ShortestDistance) {
        ClosestTriangleIndex = I;
        ShortestDistance = TestDistance;
      }
    }
  }

  trace_result Result;
  if(ClosestTriangleIndex != MEMSIZE_MAX) {
    Result.Hit = true;
    Result.Hit = ShortestDistance != FP32_MAX;
    Result.Position = Ray.Origin + Ray.Direction * ShortestDistance;
    Result.Normal = Scene->Triangles[ClosestTriangleIndex].CalcNormal();
  }
  else {
    Result.Hit = false;
  }

  return Result;
}

static v3fp32 CalcRadiance(scene const *Scene, v3fp32 Point, v3fp32 Normal, v3fp32 Direction) {
  fp32 Intensity = v3fp32::Dot(-Normal, Scene->Sun.Direction);
  v3fp32 Result(Intensity);
  return Result;
}

void Draw(frame_buffer *Buffer, scene const *Scene) {
  const static fp32 Exposure = 255.0f;

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
      v3fp32 WorldPixelPosition = WorldRowCenter + Scene->Camera.Right * (PixelColCenterX * ScreenToWorldPlaneRatio);
      v3fp32 Difference = WorldPixelPosition - Scene->Camera.Position;
      Ray.Direction = v3fp32::Normalize(Difference);

      memsize PixelIndex = ScreenPixelYOffset + X;
      v3fp32 Radiance;
      trace_result TraceResult = Trace(Scene, Ray);
      if(TraceResult.Hit) {
        Radiance = CalcRadiance(Scene, TraceResult.Position, TraceResult.Normal, -Ray.Direction);
      }
      else {
        Radiance.Clear();
      }

      color *Pixel = Buffer->Bitmap + PixelIndex;
      v3fp32 Brightness = Radiance * Exposure;
      (*Pixel).R = MinMemsize(255, RoundFP32(Brightness.X));
      (*Pixel).G = MinMemsize(255, RoundFP32(Brightness.X));
      (*Pixel).B = MinMemsize(255, RoundFP32(Brightness.X));
    }
  }
}
