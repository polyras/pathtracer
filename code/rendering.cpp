#include <new>
#include "rendering.h"
#include "lib/assert.h"

#define EXPOSURE 20
#define TILE_SIZE 16
#define SAMPLE_COUNT 32
#define BOUNCE_COUNT 1

static const fp32 Inv255 = 1.0f / 255.0f;

struct tile {
  v2ui16 Pos;
  v2ui16 Size;
};

static const v3fp32 ArbitraryDirection = v3fp32::Normalize(v3fp32(15, 1, 67));
static resolution Resolution;
static tile *Tiles = nullptr;

enum struct object_type {
  triangle,
  sphere
};

struct object_trace_result {
  bool Hit;
  object_type Type;
  memsize Index;
  fp32 Distance;
};

struct detail_trace_result {
  bool Hit;
  memsize ID;
  v3fp32 Position;
  v3fp32 Normal;
  v3fp32 Intensity;
  color Albedo;
};

struct id_trace_result {
  bool Hit;
  memsize ID;
};

scene::scene() {
  TriangleCount = 0;
}

void scene::AddTriangle(v3fp32 V0, v3fp32 V1, v3fp32 V2, color Albedo) {
  DebugAssert(TriangleCount != sizeof(Triangles) / sizeof(triangle));
  triangle *T = Triangles + TriangleCount;
  T->ID = NextObjectID++;
  T->Vertices[0] = V0;
  T->Vertices[1] = V1;
  T->Vertices[2] = V2;
  T->Albedo = Albedo;
  TriangleCount++;
}

void scene::AddSphere(v3fp32 Pos, fp32 Radius, v3fp32 Intensity, color Albedo) {
  DebugAssert(SphereCount != sizeof(Spheres) / sizeof(sphere));
  sphere *S = Spheres + SphereCount;
  S->ID = NextObjectID++;
  S->Pos = Pos;
  S->Radius = Radius;
  S->Intensity = Intensity;
  S->Albedo = Albedo;
  SphereCount++;
}

static inline v3fp32 ColorToV3FP32(color C) {
  v3fp32 Result(C.R, C.G, C.B);
  return Result;
}

static object_trace_result TraceObject(scene const *Scene, ray Ray) {
  fp32 ShortestDistance = FP32_MAX;

  object_trace_result Result = { .Hit = false };
  fp32 TestDistance;

  for(memsize I=0; I<Scene->TriangleCount; ++I) {
    triangle const *Triangle = Scene->Triangles + I;
    if(Triangle->Intersect(Ray, &TestDistance)) {
      if(TestDistance < ShortestDistance) {
        ShortestDistance = TestDistance;
        Result.Hit = true;
        Result.Type = object_type::triangle;
        Result.Index = I;
      }
    }
  }

  for(memsize I=0; I<Scene->SphereCount; ++I) {
    sphere const *Sphere = Scene->Spheres + I;
    if(Sphere->Intersect(Ray, &TestDistance)) {
      if(TestDistance < ShortestDistance) {
        ShortestDistance = TestDistance;
        Result.Hit = true;
        Result.Type = object_type::sphere;
        Result.Index = I;
      }
    }
  }

  Result.Distance = ShortestDistance;

  return Result;
}

static id_trace_result TraceID(scene const *Scene, ray Ray) {
  object_trace_result ObjectResult = TraceObject(Scene, Ray);
  id_trace_result IDResult;

  if(!ObjectResult.Hit) {
    IDResult.Hit = false;
    return IDResult;
  }

  IDResult.Hit = true;
  switch(ObjectResult.Type) {
    case object_type::triangle: {
      IDResult.ID = Scene->Triangles[ObjectResult.Index].ID;
      break;
    }
    case object_type::sphere: {
      IDResult.ID = Scene->Spheres[ObjectResult.Index].ID;
      break;
    }
    default:
      DebugAssert(false);
  }

  return IDResult;
}

static detail_trace_result TraceDetails(scene const *Scene, ray Ray) {
  object_trace_result ObjectResult = TraceObject(Scene, Ray);
  detail_trace_result DetailResult;

  if(!ObjectResult.Hit) {
    DetailResult.Hit = false;
    return DetailResult;
  }

  DetailResult.Hit = true;
  DetailResult.Position = Ray.Origin + Ray.Direction * ObjectResult.Distance;
  switch(ObjectResult.Type) {
    case object_type::triangle: {
      triangle const *Triangle = Scene->Triangles + ObjectResult.Index;
      DetailResult.Normal = Triangle->CalcNormal();
      DetailResult.Albedo = Triangle->Albedo;
      DetailResult.Intensity = v3fp32(0.0f);
      DetailResult.ID = Triangle->ID;
      break;
    }
    case object_type::sphere: {
      sphere const *Sphere = Scene->Spheres + ObjectResult.Index;
      DetailResult.Normal = Sphere->CalcNormal(DetailResult.Position);
      DetailResult.Albedo = Sphere->Albedo;
      DetailResult.Intensity = Sphere->Intensity;
      DetailResult.ID = Sphere->ID;
      break;
    }
    default:
      DebugAssert(false);
  }

  return DetailResult;
}

static v3fp32 CalcRadiance(scene const *Scene, ray Ray, memsize Depth) {
  detail_trace_result ObjectTraceResult = TraceDetails(Scene, Ray);
  if(!ObjectTraceResult.Hit) {
    return v3fp32(0.01f, 0.1f, 0.4f);
  }

  v3fp32 DirectLight(0);
  v3fp32 SunPosDifference = Scene->Sun.Position - ObjectTraceResult.Position;
  if(v3fp32::Dot(SunPosDifference, ObjectTraceResult.Normal) > 0) {
    v3fp32 SunDirection = v3fp32::Normalize(SunPosDifference);
    ray SunRay = { .Origin = ObjectTraceResult.Position, .Direction = SunDirection };
    id_trace_result SunTraceResult = TraceID(Scene, SunRay);

    if(!SunTraceResult.Hit) {
      fp32 Attenuation = v3fp32::Dot(ObjectTraceResult.Normal, SunDirection);
      DirectLight.Set(Scene->Sun.Irradiance * Attenuation);
    }
  }

  for(memsize I=0; I<Scene->SphereCount; ++I) {
    if(I == ObjectTraceResult.ID) {
      continue;
    }
    sphere const *Sphere = Scene->Spheres + I;
    v3fp32 SpatialDifference = Sphere->Pos - ObjectTraceResult.Position;
    if(v3fp32::Dot(SpatialDifference, ObjectTraceResult.Normal) < 0) {
      continue;
    }
    fp32 Distance = SpatialDifference.CalcLength();
    v3fp32 Direction = SpatialDifference / Distance;
    ray SphereLightRay = {
      .Origin = ObjectTraceResult.Position,
      .Direction = Direction
    };
    id_trace_result SphereLightTraceResult = TraceID(Scene, SphereLightRay);
    if(SphereLightTraceResult.Hit && SphereLightTraceResult.ID == Sphere->ID) {
      fp32 Attenuation = v3fp32::Dot(ObjectTraceResult.Normal, Direction) / (Distance*Distance);
      DirectLight += Sphere->Intensity * Attenuation;
    }
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

  v3fp32 Albedo = ColorToV3FP32(ObjectTraceResult.Albedo) * Inv255;
  v3fp32 ReflectedRadiance = v3fp32::Hadamard(
    DirectLight + IndirectLight,
    Albedo * PI_INV
  );
  return ReflectedRadiance + ObjectTraceResult.Intensity;
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
