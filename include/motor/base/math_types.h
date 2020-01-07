#pragma once

#include <math.h>

#if defined(_MSC_VER)
#define MT_MATH_ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
#define MT_MATH_ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define MT_MATH_ALIGNAS(x) __attribute__((aligned(x)))
#endif

#define MT_PI 3.14159265358979323846f

#define V2(x, y) ((Vec2){x, y})
#define V3(x, y, z) ((Vec3){x, y, z})
#define V4(x, y, z, w) ((Vec4){x, y, z, w})

#define MT_RAD(degrees) (degrees * (MT_PI / 180.0f))
#define MT_DEG(radians) (radians / (MT_PI / 180.0f))

#ifndef MT_MATH_DISABLE_SSE
#if defined(__x86_64__) || defined(__i386__)
#define MT_MATH_USE_SSE
#endif
#endif

#ifdef MT_MATH_USE_SSE
#include <xmmintrin.h>
#endif

typedef union Vec2 {
    struct
    {
        float x, y;
    };
    struct
    {
        float r, g;
    };
    float v[2];
} Vec2;

typedef union Vec3 {
    struct
    {
        float x, y, z;
    };
    struct
    {
        float r, g, b;
    };
    struct
    {
        Vec2 xy;
    };
    float v[3];
} Vec3;

typedef union MT_MATH_ALIGNAS(16) Vec4 {
    struct
    {
        union {
            Vec3 xyz;
            struct
            {
                float x, y, z;
            };
        };
        float w;
    };

    struct
    {
        union {
            Vec3 rgb;
            struct
            {
                float r, g, b;
            };
        };
        float a;
    };

    struct
    {
        Vec2 xy;
    };

    float v[4];
} Vec4;

typedef union MT_MATH_ALIGNAS(16) Mat4 {
    float cols[4][4];
    float elems[16];
    Vec4 v[4];
#ifdef MT_MATH_USE_SSE
    __m128 sse_cols[4];
#endif
} Mat4;

typedef union Quat {
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        Vec3 xyz;
    };
} Quat;
