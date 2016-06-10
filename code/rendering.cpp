#include "rendering.h"
#include "lib/assert.h"

#define EXPOSURE 40
#define SAMPLE_COUNT 32
#define BOUNCE_COUNT 1

static const v3fp32 ArbitraryDirection = v3fp32::Normalize(v3fp32(15, 1, 67));

struct ray {
  v3fp32 Origin;
  v3fp32 Direction;
};

struct trace_result {
  bool Hit;
  v3fp32 Position;
  v3fp32 Normal;
  color Albedo;
};

scene::scene() {
  TriangleCount = 0;
}

void scene::AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2, color Albedo) {
  triangle *T = Triangles + TriangleCount;
  T->Vertices[0] = V0;
  T->Vertices[1] = V1;
  T->Vertices[2] = V2;
  T->Albedo = Albedo;
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
  if(T < 0.0001) {
    return false;
  }

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
    triangle const *Triangle = Scene->Triangles + I;
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
    triangle const *Triangle = Scene->Triangles + ClosestTriangleIndex;
    Result.Normal = Triangle->CalcNormal();
    Result.Albedo = Triangle->Albedo;
  }
  else {
    Result.Hit = false;
  }

  return Result;
}

static v3fp32 CalcRadiance(scene const *Scene, ray Ray, memsize Depth) {
  trace_result ObjectTraceResult = Trace(Scene, Ray);
  if(!ObjectTraceResult.Hit) {
    return v3fp32(2.2f);
  }

  v3fp32 SunPosDifference = Scene->Sun.Position - ObjectTraceResult.Position;
  fp32 SunDistance = SunPosDifference.CalcLength();
  v3fp32 SunDirection = SunPosDifference / SunDistance;
  ray SunRay = { .Origin = ObjectTraceResult.Position, .Direction = SunDirection };
  trace_result SunTraceResult = Trace(Scene, SunRay);
  v3fp32 DirectLight(0);

  if(!SunTraceResult.Hit) {
    DirectLight = v3fp32(MaxFP32(0, v3fp32::Dot(ObjectTraceResult.Normal, SunDirection)));
    DirectLight *= Scene->Sun.Irradiance;
  }

  v3fp32 IndirectLight(0);
  if(Depth != BOUNCE_COUNT) {
    m33fp32 Rotation;
    Rotation.Col1 =  ArbitraryDirection - ObjectTraceResult.Normal * v3fp32::Dot(ArbitraryDirection, ObjectTraceResult.Normal);
    Rotation.Col1.Normalize();
    Rotation.Col3 = ObjectTraceResult.Normal;
    Rotation.Col2 = v3fp32::Cross(Rotation.Col1, Rotation.Col3);

    ray SampleRay;
    SampleRay.Origin = ObjectTraceResult.Position;
    for(memsize I=0; I<SAMPLE_COUNT; ++I) {
      fp32 Random1 = drand48();
      fp32 Random2 = drand48();
      fp32 R = SqrtFP32(1.0f - Random1 * Random1);
      fp32 Phi = M_PI * 2.0f * Random2;

      SampleRay.Direction.Set(
        CosFP32(Phi) * R,
        SinFP32(Phi) * R,
        Random1
      );

      SampleRay.Direction = Rotation * SampleRay.Direction;
      IndirectLight += CalcRadiance(Scene, SampleRay, Depth + 1) * v3fp32::Dot(ObjectTraceResult.Normal, SampleRay.Direction);
    }
    IndirectLight *= (2.0f * M_PI) / SAMPLE_COUNT;
  }

  v3fp32 Albedo(
    static_cast<fp32>(ObjectTraceResult.Albedo.R) / 255.0f,
    static_cast<fp32>(ObjectTraceResult.Albedo.G) / 255.0f,
    static_cast<fp32>(ObjectTraceResult.Albedo.B) / 255.0f
  );
  v3fp32 Result = v3fp32::Hadamard(
    DirectLight + IndirectLight,
    Albedo * PI_INV
  );
  return Result;
}

void Draw(frame_buffer *Buffer, scene const *Scene) {
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

      v3fp32 Radiance = CalcRadiance(Scene, Ray, 0);
      v3fp32 Brightness = Radiance * EXPOSURE;
      color *Pixel = Buffer->Bitmap + ScreenPixelYOffset + X;
      (*Pixel).R = MinMemsize(255, RoundFP32(Brightness.X));
      (*Pixel).G = MinMemsize(255, RoundFP32(Brightness.Y));
      (*Pixel).B = MinMemsize(255, RoundFP32(Brightness.Z));

      // Temp safe guard:
      ReleaseAssert((*Pixel).R != 255, "Over exposure");
      ReleaseAssert((*Pixel).G != 255, "Over exposure");
      ReleaseAssert((*Pixel).B != 255, "Over exposure");
    }
  }
}
