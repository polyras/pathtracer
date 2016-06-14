#pragma once

#include <stdio.h>
#include <stdlib.h>

#if DEBUG
void _DebugAssert(bool Expression, const char *Filename, size_t Line);
#define DebugAssert(Expression) _DebugAssert(Expression, __FILE__, __LINE__)
#else
#define DebugAssert(Expression)
#endif

void ReleaseAssert(bool Expression, const char *Message);
#define InvalidCodePath DebugAssert(false)
