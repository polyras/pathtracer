void InitScene1(scene *Scene) {
  camera *Cam = &Scene->Camera;
  Cam->Position.Set(0, 0.7, 0);
  Cam->Direction.Set(0, -0.1, 1);
  Cam->Direction.Normalize();
  Cam->Right.Set(1, 0, 0);
  Cam->FOV = DegToRad(60);

  // front
  Scene->AddTriangle(
    v3fp32(-0.5f, 1.0f, 4.0f), // top left
    v3fp32(-0.5f, 0.0f, 4.0f), // left foot
    v3fp32(0.5f, 0.0f, 4.0f), // right foot
    color(255, 20, 20)
  );
  Scene->AddTriangle(
    v3fp32(-0.5f, 1.0f, 4.0f), // top left
    v3fp32(0.5f, 0.0f, 4.0f), // bottom right
    v3fp32(0.5f, 1.0f, 4.0f), // top right
    color(255, 20, 20)
  );

  // side
  Scene->AddTriangle(
    v3fp32(-0.5f, 0.0f, 5.0f),
    v3fp32(-0.5f, 0.0f, 4.0f),
    v3fp32(-0.5f, 1.0f, 4.0f),
    color(255, 20, 20)
  );
  Scene->AddTriangle(
    v3fp32(-0.5f, 1.0f, 5.0f), // top left
    v3fp32(-0.5f, 0.0f, 5.0f), // bottom right
    v3fp32(-0.5f, 1.0f, 4.0f), // top right
    color(255, 20, 20)
  );

  // Ground
  Scene->AddTriangle(
    v3fp32(0.0f, 0.0f, -20.0f),
    v3fp32(100.0f, 0.0f, 50.0f),
    v3fp32(-100.0f, 0.0f, 50.0f),
    color(255, 255, 255)
  );

  Scene->Sun.Position.Set(10.0f, 10.0f, 0.0f);
  Scene->Sun.Position.Set(20.0, 10, -20);
  Scene->Sun.Irradiance = 15.0f;
}
