// RaylibMath.h
//
// Includes raylib's raymath.h (matrices and vectors). raymath.h is C code
// whose `= { 0 }` initialisers trip GCC/Clang's -Wextra warnings in C++,
// so those are switched off just for this include. MSVC does not warn
// about them and does not understand the GCC pragma, so it gets a plain
// include.

#pragma once

#if defined(__GNUC__) // GCC and Clang (including Apple's)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#else
#include "raymath.h"
#endif
