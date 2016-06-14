#include "rendering.h"

struct game_button {
  ui8 ChangeCount;
  bool Pressed;
};

union game_input {
  game_button States[4];
  struct {
    game_button Left;
    game_button Right;
    game_button Up;
    game_button Down;
  };
};

void InitGame(scene *Scene);
void UpdateGame(scene *Scene, game_input *Input, uusec64 TimeDelta);
