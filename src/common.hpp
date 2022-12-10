#pragma once

#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

#define vec vector

#define FMT_HEADER_ONLY
#include "fmt/core.h"

#if defined(PLATFORM_WEB)
#define CUSTOM_MODAL_DIALOGS
#include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define TRANSPARENT Color {0,0,0,0}

// https://stackoverflow.com/questions/8487986/file-macro-shows-full-path/8488201#8488201
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

using namespace std;

#define let auto
#define self this

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef double f32;
typedef float f64;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256

extern f64 screenScale; 
extern f64 prevScreenScale;

[[maybe_unused]] void static _panic(char const *str, char const* file, int const lineno) {
  fmt::print("PANIC: {}, File: {} Line: {}\n", str, file, lineno);
}

#define panic(msg) _panic(msg, __FILENAME__, __LINE__)

template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})
