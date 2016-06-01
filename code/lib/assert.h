#pragma once

#include <stdio.h>
#include <stdlib.h>

void _DebugAssert(bool Expression, const char *Filename, size_t Line);
#define DebugAssert(Expression) _DebugAssert(Expression, __FILE__, __LINE__)
void ReleaseAssert(bool Expression, const char *Message);
#define InvalidCodePath DebugAssert(false)
