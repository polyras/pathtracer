#include "game.h"

static void AddQuad(
  scene *Scene,
  v3fp32 Pos,
  color Color,
  v3fp32 Corner1, v3fp32 Corner2, v3fp32 Corner3, v3fp32 Corner4
) {
  Scene->AddTriangle(
    Pos + Corner1,
    Pos + Corner2,
    Pos + Corner3,
    Color
  );
  Scene->AddTriangle(
    Pos + Corner3,
    Pos + Corner2,
    Pos + Corner4,
    Color
  );
}


static void AddCube(scene *Scene, fp32 Size, v3fp32 Pos, color Color) {
  // front upper
  AddQuad(
    Scene, Pos, Color,
    v3fp32(-0.5f * Size, 0.5f * Size, -0.5f * Size),
    v3fp32(-0.5f * Size, -0.5f * Size, -0.5f * Size),
    v3fp32(0.5f * Size, 0.5f * Size, -0.5f * Size),
    v3fp32(0.5f * Size, -0.5f * Size, -0.5f * Size)
  );

  // left
  AddQuad(
    Scene, Pos, Color,
    v3fp32(-0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(-0.5f * Size, -0.5f * Size, 0.5f * Size),
    v3fp32(-0.5f * Size, 0.5f * Size, -0.5f * Size),
    v3fp32(-0.5f * Size, -0.5f * Size, -0.5f * Size)
  );

  // right
  AddQuad(
    Scene, Pos, Color,
    v3fp32(0.5f * Size, 0.5f * Size, -0.5f * Size),
    v3fp32(0.5f * Size, -0.5f * Size, -0.5f * Size),
    v3fp32(0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(0.5f * Size, -0.5f * Size, 0.5f * Size)
  );

  // top
  AddQuad(
    Scene, Pos, Color,
    v3fp32(-0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(-0.5f * Size, 0.5f * Size, -0.5f * Size),
    v3fp32(0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(0.5f * Size, 0.5f * Size, -0.5f * Size)
  );

  // back
  AddQuad(
    Scene, Pos, Color,
    v3fp32(-0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(0.5f * Size, 0.5f * Size, 0.5f * Size),
    v3fp32(-0.5f * Size, -0.5f * Size, 0.5f * Size),
    v3fp32(0.5f * Size, -0.5f * Size, 0.5f * Size)
  );
}

static void SetupGreenBox(scene *Scene) {
  v3fp32 Pos(-1.5f, 0.5f, 4.5f);
  color Color(30, 210, 30);
  AddCube(Scene, 1.0f, Pos, Color);
}

static void SetupOrangeBox(scene *Scene) {
  v3fp32 Pos(1.5f, 0.5f, 4.5f);
  color Color(255, 140, 10);
  AddCube(Scene, 1.0f, Pos, Color);
}

static void SetupRedBox(scene *Scene) {
  v3fp32 Pos(0.0f, 0.5f, 4.5f);
  color Color(255, 20, 20);
  AddCube(Scene, 1.0f, Pos, Color);
}

static void SetupGround(scene *Scene) {
  Scene->AddTriangle(
    v3fp32(0.0f, 0.0f, -20.0f),
    v3fp32(100.0f, 0.0f, 50.0f),
    v3fp32(-100.0f, 0.0f, 50.0f),
    color(255, 255, 255)
  );
}

void InitGame(scene *Scene) {
  camera *Cam = &Scene->Camera;
  Cam->Position.Set(0.0f, 1.6f, 0.0f);
  Cam->Direction.Set(0.0f, -0.2f, 1.0f);
  Cam->Direction.Normalize();
  Cam->Right.Set(1.0f, 0.0f, 0.0f);
  Cam->FOV = DegToRad(60.0f);

  SetupGreenBox(Scene);
  SetupRedBox(Scene);
  SetupOrangeBox(Scene);

  // Ground
  SetupGround(Scene);

  Scene->Sun.Position.Set(5.0f, 5.0f, -20.0f);
  Scene->Sun.Irradiance = 15.0f;
}

void UpdateGame(scene *Scene, game_input *Input, uusec64 TimeDelta) {
  v3fp32 MoveDirection(0.0f);
  if(Input->Left.Pressed) {
    MoveDirection.X += -1.0f;
  }
  if(Input->Right.Pressed) {
    MoveDirection.X += 1.0f;
  }
  if(Input->Up.Pressed) {
    MoveDirection.Y += 1.0f;
  }
  if(Input->Down.Pressed) {
    MoveDirection.Y += -1.0f;
  }
  if(!MoveDirection.IsZero()) {
    MoveDirection.Normalize();
    v3fp32 Movement = MoveDirection * (0.000001 * TimeDelta);
    Scene->Camera.Position += Movement;
  }
}
