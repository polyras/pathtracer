#include "assert.h"

void _DebugAssert(bool Expression, char const *Filename, size_t Line) {
  if(!Expression) {
    fprintf(stderr, "Assert failed: %s:%zu\n", Filename, Line);
    exit(1);
  }
}

void ReleaseAssert(bool Expression, char const *Message) {
  if(!Expression) {
    fprintf(stderr, "Assert failed: %s\n", Message);
    exit(1);
  }
}
