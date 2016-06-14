#include <new>
#include "rendering.h"
#include "lib/assert.h"

#define EXPOSURE 30
#define TILE_SIZE 16
#define SAMPLE_COUNT 32
#define BOUNCE_COUNT 1

struct tile {
  v2ui16 Pos;
  v2ui16 Size;
};

static const v3fp32 ArbitraryDirection = v3fp32::Normalize(v3fp32(15, 1, 67));
static resolution Resolution;
static tile *Tiles = nullptr;

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
  DebugAssert(TriangleCount != sizeof(Triangles) / sizeof(triangle));
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
  v3fp32 SunDirection = v3fp32::Normalize(SunPosDifference);
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

memsize InitRendering(resolution AResolution) {
  Resolution = AResolution;

  memsize TileHorizontalCount = (Resolution.Dimension.X + TILE_SIZE - 1) / TILE_SIZE;
  memsize TileVerticalCount = (Resolution.Dimension.Y + TILE_SIZE - 1) / TILE_SIZE;
  memsize TileCount = TileHorizontalCount * TileVerticalCount;
  Tiles = new (std::nothrow) tile[TileCount];
  ReleaseAssert(Tiles != nullptr, "Could not allocate tiles.");

  memsize X = 0, Y = 0;
  for(memsize I=0; I<TileCount; ++I) {
    DebugAssert(Y <= Resolution.Dimension.Y);
    DebugAssert(X <= Resolution.Dimension.X);

    tile *Tile = Tiles + I;
    Tile->Pos.Set(X, Y);
    memsize RemainingWidth = Resolution.Dimension.X - X;
    Tile->Size.X = MinMemsize(TILE_SIZE, RemainingWidth);

    memsize RemainingHeight = Resolution.Dimension.Y - Y;
    Tile->Size.Y = MinMemsize(TILE_SIZE, RemainingHeight);

    X += Tile->Size.X;
    if(X == Resolution.Dimension.X) {
      X = 0;
      Y += TILE_SIZE;
    }
  }

  return TileCount;
}

void TerminateRendering() {
  delete[] Tiles;
  Tiles = nullptr;
}

void RenderTile(color *Buffer, scene const *Scene, memsize TileIndex) {
  tile *Tile = Tiles + TileIndex;

  v3fp32 WorldPlaneCenter = Scene->Camera.Position + Scene->Camera.Direction;
  fp32 WorldPlaneWidth = TanFP32(Scene->Camera.FOV/2.0f)*2.0f;

  ui16 ScreenPlaneWidth = Resolution.Dimension.X;
  ui16 ScreenPlaneHeight = Resolution.Dimension.Y;
  ui16 HalfScreenPlaneWidth = ScreenPlaneWidth * 0.5f;
  ui16 HalfScreenPlaneHeight = ScreenPlaneHeight * 0.5f;

  fp32 ScreenToWorldPlaneRatio = static_cast<fp32>(WorldPlaneWidth) / (ScreenPlaneWidth);
  ray Ray = { .Origin = Scene->Camera.Position };
  v3fp32 Up(0, 1, 0);

  ui32 ScreenPixelYOffset;
  ui16 EndX = Tile->Pos.X + Tile->Size.X;
  ui16 EndY = Tile->Pos.Y + Tile->Size.Y;
  for(ui16 Y=Tile->Pos.Y; Y<EndY; ++Y) {
    ScreenPixelYOffset = Y * ScreenPlaneWidth;
    fp32 ScreenRowCenterY = 0.5f + (static_cast<si16>(Y) - HalfScreenPlaneHeight);
    v3fp32 WorldRowCenter = WorldPlaneCenter + Up * (ScreenRowCenterY * ScreenToWorldPlaneRatio);
    for(ui16 X=Tile->Pos.X; X<EndX; ++X) {
      fp32 PixelColCenterX = 0.5f + (static_cast<si16>(X) - HalfScreenPlaneWidth);
      v3fp32 WorldPixelPosition = WorldRowCenter + Scene->Camera.Right * (PixelColCenterX * ScreenToWorldPlaneRatio);
      v3fp32 Difference = WorldPixelPosition - Scene->Camera.Position;
      Ray.Direction = v3fp32::Normalize(Difference);

      v3fp32 Radiance = CalcRadiance(Scene, Ray, 0);
      v3fp32 Brightness = Radiance * EXPOSURE;
      color *Pixel = Buffer + ScreenPixelYOffset + X;
      (*Pixel).R = MinMemsize(255, RoundFP32(Brightness.X));
      (*Pixel).G = MinMemsize(255, RoundFP32(Brightness.Y));
      (*Pixel).B = MinMemsize(255, RoundFP32(Brightness.Z));

      // Temp safe guard:
      DebugAssert((*Pixel).R != 255);
      DebugAssert((*Pixel).G != 255);
      DebugAssert((*Pixel).B != 255);
    }
  }
}
