#pragma once

#include "math_types.h"
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MT_MATH_INLINE static inline

/*
 * Vec2 functions
 */

MT_MATH_INLINE Vec2 v2_zero()
{
    return (Vec2){{0.0f, 0.0f}};
}

MT_MATH_INLINE Vec2 v2_one()
{
    return (Vec2){{1.0f, 1.0f}};
}

MT_MATH_INLINE float v2_mag(Vec2 vec)
{
    return sqrtf((vec.x * vec.x) + (vec.y * vec.y));
}

MT_MATH_INLINE Vec2 v2_add(Vec2 left, Vec2 right)
{
    Vec2 result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    return result;
}

MT_MATH_INLINE Vec2 v2_adds(Vec2 left, float right)
{
    Vec2 result;
    result.x = left.x + right;
    result.y = left.y + right;
    return result;
}

MT_MATH_INLINE Vec2 v2_sub(Vec2 left, Vec2 right)
{
    Vec2 result;
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    return result;
}

MT_MATH_INLINE Vec2 v2_subs(Vec2 left, float right)
{
    Vec2 result;
    result.x = left.x - right;
    result.y = left.y - right;
    return result;
}

MT_MATH_INLINE Vec2 v2_mul(Vec2 left, Vec2 right)
{
    Vec2 result;
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    return result;
}

MT_MATH_INLINE Vec2 v2_muls(Vec2 left, float right)
{
    Vec2 result;
    result.x = left.x * right;
    result.y = left.y * right;
    return result;
}

MT_MATH_INLINE Vec2 v2_div(Vec2 left, Vec2 right)
{
    Vec2 result;
    result.x = left.x / right.x;
    result.y = left.y / right.y;
    return result;
}

MT_MATH_INLINE Vec2 v2_divs(Vec2 left, float right)
{
    Vec2 result;
    result.x = left.x / right;
    result.y = left.y / right;
    return result;
}

MT_MATH_INLINE float v2_distance(Vec2 left, Vec2 right)
{
    return v2_mag(v2_sub(left, right));
}

// TESTED
MT_MATH_INLINE float v2_dot(Vec2 left, Vec2 right)
{
    return (left.x * right.x) + (left.y * right.y);
}

/*
 * Vec3 functions
 */

MT_MATH_INLINE Vec3 v3_zero()
{
    return (Vec3){{0.0f, 0.0f, 0.0f}};
}

MT_MATH_INLINE Vec3 v3_one()
{
    return (Vec3){{1.0f, 1.0f, 1.0f}};
}

MT_MATH_INLINE float v3_mag(Vec3 vec)
{
    return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

MT_MATH_INLINE Vec3 v3_add(Vec3 left, Vec3 right)
{
    Vec3 result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    return result;
}

MT_MATH_INLINE Vec3 v3_adds(Vec3 left, float right)
{
    Vec3 result;
    result.x = left.x + right;
    result.y = left.y + right;
    result.z = left.z + right;
    return result;
}

MT_MATH_INLINE Vec3 v3_sub(Vec3 left, Vec3 right)
{
    Vec3 result;
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    return result;
}

MT_MATH_INLINE Vec3 v3_subs(Vec3 left, float right)
{
    Vec3 result;
    result.x = left.x - right;
    result.y = left.y - right;
    result.z = left.z - right;
    return result;
}

MT_MATH_INLINE Vec3 v3_mul(Vec3 left, Vec3 right)
{
    Vec3 result;
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    return result;
}

MT_MATH_INLINE Vec3 v3_muls(Vec3 left, float right)
{
    Vec3 result;
    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    return result;
}

MT_MATH_INLINE Vec3 v3_div(Vec3 left, Vec3 right)
{
    Vec3 result;
    result.x = left.x / right.x;
    result.y = left.y / right.y;
    result.z = left.z / right.z;
    return result;
}

MT_MATH_INLINE Vec3 v3_divs(Vec3 left, float right)
{
    Vec3 result;
    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    return result;
}

MT_MATH_INLINE float v3_distance(Vec3 left, Vec3 right)
{
    return v3_mag(v3_sub(left, right));
}

MT_MATH_INLINE float v3_dot(Vec3 left, Vec3 right)
{
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

// TODO: test
MT_MATH_INLINE Vec3 v3_cross(Vec3 left, Vec3 right)
{
    Vec3 result;
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);
    return result;
}

// TODO: test with zero norm vector
MT_MATH_INLINE Vec3 v3_normalize(Vec3 vec)
{
    Vec3 result = vec;
    float norm = sqrtf(v3_dot(vec, vec));
    if (norm != 0.0f)
    {
        result = v3_muls(vec, 1.0f / norm);
    }
    return result;
}

/*
 * Vec4 functions
 */

MT_MATH_INLINE Vec4 v4_zero()
{
    return (Vec4){{0.0f, 0.0f, 0.0f, 0.0f}};
}

MT_MATH_INLINE Vec4 v4_one()
{
    return (Vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
}

// TESTED
MT_MATH_INLINE Vec4 v4_add(Vec4 left, Vec4 right)
{
    Vec4 result;
#ifdef MT_MATH_USE_SSE
    _mm_store_ps(
        (float *)&result, _mm_add_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    result.w = left.w + right.w;
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Vec4 v4_mul(Vec4 left, Vec4 right)
{
    Vec4 result;
#ifdef MT_MATH_USE_SSE
    _mm_store_ps(
        (float *)&result, _mm_mul_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    result.w = left.w * right.w;
#endif
    return result;
}

MT_MATH_INLINE Vec4 v4_muls(Vec4 left, float right)
{
    return v4_mul(left, (Vec4){{right, right, right, right}});
}

// TESTED
MT_MATH_INLINE float v4_dot(Vec4 left, Vec4 right)
{
    float result;
#ifdef MT_MATH_USE_SSE
    __m128 rone = _mm_mul_ps(*((__m128 *)&left), *((__m128 *)&right));
    __m128 rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(2, 3, 0, 1));
    rone = _mm_add_ps(rone, rtwo);
    rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(0, 1, 2, 3));
    rone = _mm_add_ps(rone, rtwo);
    _mm_store_ss(&result, rone);
#else
    result = (left.x * right.x) + (left.y * right.y) + (left.z * right.z) + (left.w * right.w);
#endif
    return result;
}

/*
 * Mat4 functions
 */

// TESTED
MT_MATH_INLINE Mat4 mat4_zero()
{
    return (Mat4){{
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    }};
}

// TESTED
MT_MATH_INLINE Mat4 mat4_diagonal(float f)
{
    return (Mat4){{
        {f, 0, 0, 0},
        {0, f, 0, 0},
        {0, 0, f, 0},
        {0, 0, 0, f},
    }};
}

// TESTED
MT_MATH_INLINE Mat4 mat4_identity()
{
    return mat4_diagonal(1.0f);
}

// TESTED
MT_MATH_INLINE Mat4 mat4_transpose(Mat4 mat)
{
    Mat4 result = mat;
#ifdef MT_MATH_USE_SSE
    _MM_TRANSPOSE4_PS(
        result.sse_cols[0], result.sse_cols[1], result.sse_cols[2], result.sse_cols[3]);
#else
    result.cols[0][1] = mat.cols[1][0];
    result.cols[0][2] = mat.cols[2][0];
    result.cols[0][3] = mat.cols[3][0];

    result.cols[1][0] = mat.cols[0][1];
    result.cols[1][2] = mat.cols[2][1];
    result.cols[1][3] = mat.cols[3][1];

    result.cols[2][0] = mat.cols[0][2];
    result.cols[2][1] = mat.cols[1][2];
    result.cols[2][3] = mat.cols[3][2];

    result.cols[3][0] = mat.cols[0][3];
    result.cols[3][1] = mat.cols[1][3];
    result.cols[3][2] = mat.cols[2][3];
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_add(Mat4 left, Mat4 right)
{
    Mat4 result;
#ifdef MT_MATH_USE_SSE
    result.sse_cols[0] = _mm_add_ps(left.sse_cols[0], right.sse_cols[0]);
    result.sse_cols[1] = _mm_add_ps(left.sse_cols[1], right.sse_cols[1]);
    result.sse_cols[2] = _mm_add_ps(left.sse_cols[2], right.sse_cols[2]);
    result.sse_cols[3] = _mm_add_ps(left.sse_cols[3], right.sse_cols[3]);
#else
    for (unsigned char i = 0; i < 16; i++)
    {
        result.elems[i] = left.elems[i] + right.elems[i];
    }
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_sub(Mat4 left, Mat4 right)
{
    Mat4 result;
#ifdef MT_MATH_USE_SSE
    result.sse_cols[0] = _mm_sub_ps(left.sse_cols[0], right.sse_cols[0]);
    result.sse_cols[1] = _mm_sub_ps(left.sse_cols[1], right.sse_cols[1]);
    result.sse_cols[2] = _mm_sub_ps(left.sse_cols[2], right.sse_cols[2]);
    result.sse_cols[3] = _mm_sub_ps(left.sse_cols[3], right.sse_cols[3]);
#else
    for (unsigned char i = 0; i < 16; i++)
    {
        result.elems[i] = left.elems[i] - right.elems[i];
    }
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_muls(Mat4 left, float right)
{
    Mat4 result;
#ifdef MT_MATH_USE_SSE
    __m128 sse_scalar = _mm_load_ps1(&right);
    result.sse_cols[0] = _mm_mul_ps(left.sse_cols[0], sse_scalar);
    result.sse_cols[1] = _mm_mul_ps(left.sse_cols[1], sse_scalar);
    result.sse_cols[2] = _mm_mul_ps(left.sse_cols[2], sse_scalar);
    result.sse_cols[3] = _mm_mul_ps(left.sse_cols[3], sse_scalar);
#else
    for (unsigned char i = 0; i < 16; i++)
    {
        result.elems[i] = left.elems[i] * right;
    }
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_divs(Mat4 left, float right)
{
    Mat4 result;
#ifdef MT_MATH_USE_SSE
    __m128 sse_scalar = _mm_load_ps1(&right);
    result.sse_cols[0] = _mm_div_ps(left.sse_cols[0], sse_scalar);
    result.sse_cols[1] = _mm_div_ps(left.sse_cols[1], sse_scalar);
    result.sse_cols[2] = _mm_div_ps(left.sse_cols[2], sse_scalar);
    result.sse_cols[3] = _mm_div_ps(left.sse_cols[3], sse_scalar);
#else
    for (unsigned char i = 0; i < 16; i++)
    {
        result.elems[i] = left.elems[i] / right;
    }
#endif
    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_mul(Mat4 left, Mat4 right)
{
    Mat4 result;
#ifdef MT_MATH_USE_SSE
    for (int i = 0; i < 4; i++)
    {
        __m128 brod1 = _mm_set1_ps(left.elems[4 * i + 0]);
        __m128 brod2 = _mm_set1_ps(left.elems[4 * i + 1]);
        __m128 brod3 = _mm_set1_ps(left.elems[4 * i + 2]);
        __m128 brod4 = _mm_set1_ps(left.elems[4 * i + 3]);
        __m128 row = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(brod1, right.sse_cols[0]), _mm_mul_ps(brod2, right.sse_cols[1])),
            _mm_add_ps(_mm_mul_ps(brod3, right.sse_cols[2]), _mm_mul_ps(brod4, right.sse_cols[3])));
        _mm_store_ps(&result.elems[4 * i], row);
    }
#else
    result = (Mat4){{{0}}};
    for (unsigned char i = 0; i < 4; i++)
    {
        for (unsigned char j = 0; j < 4; j++)
        {
            for (unsigned char p = 0; p < 4; p++)
            {
                result.cols[i][j] += left.cols[i][p] * right.cols[p][j];
            }
        }
    }
#endif
    return result;
}

// TODO: test
MT_MATH_INLINE Vec4 mat4_mulv(Mat4 left, Vec4 right)
{
    // TODO: SIMD version

    Vec4 result;

    result.v[0] = left.cols[0][0] * right.v[0] + left.cols[1][0] * right.v[1] +
                  left.cols[2][0] * right.v[2] + left.cols[3][0] * right.v[3];
    result.v[1] = left.cols[0][1] * right.v[0] + left.cols[1][1] * right.v[1] +
                  left.cols[2][1] * right.v[2] + left.cols[3][1] * right.v[3];
    result.v[2] = left.cols[0][2] * right.v[0] + left.cols[1][2] * right.v[1] +
                  left.cols[2][2] * right.v[2] + left.cols[3][2] * right.v[3];
    result.v[3] = left.cols[0][3] * right.v[0] + left.cols[1][3] * right.v[1] +
                  left.cols[2][3] * right.v[2] + left.cols[3][3] * right.v[3];

    return result;
}

// TESTED
MT_MATH_INLINE Mat4 mat4_inverse(Mat4 mat)
{
    // TODO: SIMD version

    Mat4 inv = {0};

    float t[6];
    float a = mat.cols[0][0], b = mat.cols[0][1], c = mat.cols[0][2], d = mat.cols[0][3],
          e = mat.cols[1][0], f = mat.cols[1][1], g = mat.cols[1][2], h = mat.cols[1][3],
          i = mat.cols[2][0], j = mat.cols[2][1], k = mat.cols[2][2], l = mat.cols[2][3],
          m = mat.cols[3][0], n = mat.cols[3][1], o = mat.cols[3][2], p = mat.cols[3][3];

    t[0] = k * p - o * l;
    t[1] = j * p - n * l;
    t[2] = j * o - n * k;
    t[3] = i * p - m * l;
    t[4] = i * o - m * k;
    t[5] = i * n - m * j;

    inv.cols[0][0] = f * t[0] - g * t[1] + h * t[2];
    inv.cols[1][0] = -(e * t[0] - g * t[3] + h * t[4]);
    inv.cols[2][0] = e * t[1] - f * t[3] + h * t[5];
    inv.cols[3][0] = -(e * t[2] - f * t[4] + g * t[5]);

    inv.cols[0][1] = -(b * t[0] - c * t[1] + d * t[2]);
    inv.cols[1][1] = a * t[0] - c * t[3] + d * t[4];
    inv.cols[2][1] = -(a * t[1] - b * t[3] + d * t[5]);
    inv.cols[3][1] = a * t[2] - b * t[4] + c * t[5];

    t[0] = g * p - o * h;
    t[1] = f * p - n * h;
    t[2] = f * o - n * g;
    t[3] = e * p - m * h;
    t[4] = e * o - m * g;
    t[5] = e * n - m * f;

    inv.cols[0][2] = b * t[0] - c * t[1] + d * t[2];
    inv.cols[1][2] = -(a * t[0] - c * t[3] + d * t[4]);
    inv.cols[2][2] = a * t[1] - b * t[3] + d * t[5];
    inv.cols[3][2] = -(a * t[2] - b * t[4] + c * t[5]);

    t[0] = g * l - k * h;
    t[1] = f * l - j * h;
    t[2] = f * k - j * g;
    t[3] = e * l - i * h;
    t[4] = e * k - i * g;
    t[5] = e * j - i * f;

    inv.cols[0][3] = -(b * t[0] - c * t[1] + d * t[2]);
    inv.cols[1][3] = a * t[0] - c * t[3] + d * t[4];
    inv.cols[2][3] = -(a * t[1] - b * t[3] + d * t[5]);
    inv.cols[3][3] = a * t[2] - b * t[4] + c * t[5];

    float det = a * inv.cols[0][0] + b * inv.cols[1][0] + c * inv.cols[2][0] + d * inv.cols[3][0];

    inv = mat4_muls(inv, 1.0f / det);

    return inv;
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 mat4_perspective(float fovy, float aspect, float n, float f)
{
    float c = 1.0f / tan(fovy / 2.0f);

    // clang-format off
    return (Mat4){{
        {c / aspect, 0.0f,  0.0f,                     0.0f},
        {0.0f,       c,     0.0f,                     0.0f},
        {0.0f,       0.0f, -(f + n) / (f - n),       -1.0f},
        {0.0f,       0.0f, -(2.0f * f * n) / (f - n), 0.0f},
    }};
    // clang-format on
}

// TODO: test
MT_MATH_INLINE Mat4 mat4_orthographic(float l, float r, float b, float t, float n, float f)
{
    // clang-format off
    return (Mat4){{
        {2.0f / (r - l),      0.0f,               0.0f,           0.0f},
        {0.0f,                2.0f / (t - b),     0.0f,           0.0f},
        {0.0f,                0.0f,               1.0f / (f - n), 0.0f},
        {-(r + l) / (r - l), -(t + b) / (t - b), -n / (f - n),    1.0f},
    }};
    // clang-format on
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 f = v3_normalize(v3_sub(center, eye));
    Vec3 s = v3_normalize(v3_cross(f, up));
    Vec3 u = v3_cross(s, f);

    Mat4 result = mat4_identity();

    result.cols[0][0] = s.x;
    result.cols[1][0] = s.y;
    result.cols[2][0] = s.z;

    result.cols[0][1] = u.x;
    result.cols[1][1] = u.y;
    result.cols[2][1] = u.z;

    result.cols[0][2] = -f.x;
    result.cols[1][2] = -f.y;
    result.cols[2][2] = -f.z;

    result.cols[3][0] = -v3_dot(s, eye);
    result.cols[3][1] = -v3_dot(u, eye);
    result.cols[3][2] = v3_dot(f, eye);

    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Quat mat4_to_quat(Mat4 mat)
{
    Quat result;
    float trace = mat.cols[0][0] + mat.cols[1][1] + mat.cols[2][2];
    if (trace > 0.0f)
    {
        float s = sqrtf(1.0f + trace) * 2.0f;
        result.w = 0.25f * s;
        result.x = (mat.cols[1][2] - mat.cols[2][1]) / s;
        result.y = (mat.cols[2][0] - mat.cols[0][2]) / s;
        result.z = (mat.cols[0][1] - mat.cols[1][0]) / s;
    }
    else if (mat.cols[0][0] > mat.cols[1][1] && mat.cols[0][0] > mat.cols[2][2])
    {
        float s = sqrtf(1.0f + mat.cols[0][0] - mat.cols[1][1] - mat.cols[2][2]) * 2.0f;
        result.w = (mat.cols[1][2] - mat.cols[2][1]) / s;
        result.x = 0.25f * s;
        result.y = (mat.cols[1][0] + mat.cols[0][1]) / s;
        result.z = (mat.cols[2][0] + mat.cols[0][2]) / s;
    }
    else if (mat.cols[1][1] > mat.cols[2][2])
    {
        float s = sqrtf(1.0f + mat.cols[1][1] - mat.cols[0][0] - mat.cols[2][2]) * 2.0f;
        result.w = (mat.cols[2][0] - mat.cols[0][2]) / s;
        result.x = (mat.cols[1][0] + mat.cols[0][1]) / s;
        result.y = 0.25f * s;
        result.z = (mat.cols[2][1] + mat.cols[1][2]) / s;
    }
    else
    {
        float s = sqrtf(1.0f + mat.cols[2][2] - mat.cols[0][0] - mat.cols[1][1]) * 2.0f;
        result.w = (mat.cols[0][1] - mat.cols[1][0]) / s;
        result.x = (mat.cols[2][0] + mat.cols[0][2]) / s;
        result.y = (mat.cols[2][1] + mat.cols[1][2]) / s;
        result.z = 0.25f * s;
    }
    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 mat4_translate(Mat4 mat, Vec3 translation)
{
    Mat4 result = mat;
    result.cols[3][0] += translation.x;
    result.cols[3][1] += translation.y;
    result.cols[3][2] += translation.z;
    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 mat4_scale(Mat4 mat, Vec3 scale)
{
    Mat4 result = mat;
    result.cols[0][0] *= scale.x;
    result.cols[1][1] *= scale.y;
    result.cols[2][2] *= scale.z;
    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 mat4_rotate(Mat4 mat, float angle, Vec3 axis)
{
    float c = cosf(angle);
    float s = sinf(angle);

    axis = v3_normalize(axis);
    Vec3 temp = v3_muls(axis, (1.0f - c));

    Mat4 rotate;
    rotate.cols[0][0] = c + temp.x * axis.x;
    rotate.cols[0][1] = temp.x * axis.y + s * axis.z;
    rotate.cols[0][2] = temp.x * axis.z - s * axis.y;

    rotate.cols[1][0] = temp.y * axis.x - s * axis.z;
    rotate.cols[1][1] = c + temp.y * axis.y;
    rotate.cols[1][2] = temp.y * axis.z + s * axis.x;

    rotate.cols[2][0] = temp.z * axis.x + s * axis.y;
    rotate.cols[2][1] = temp.z * axis.y - s * axis.x;
    rotate.cols[2][2] = c + temp.z * axis.z;

    Mat4 result;
    result.v[0] = v4_add(
        v4_add(v4_muls(mat.v[0], rotate.cols[0][0]), v4_muls(mat.v[1], rotate.cols[0][1])),
        v4_muls(mat.v[2], rotate.cols[0][2]));
    result.v[1] = v4_add(
        v4_add(v4_muls(mat.v[0], rotate.cols[1][0]), v4_muls(mat.v[1], rotate.cols[1][1])),
        v4_muls(mat.v[2], rotate.cols[1][2]));
    result.v[2] = v4_add(
        v4_add(v4_muls(mat.v[0], rotate.cols[2][0]), v4_muls(mat.v[1], rotate.cols[2][1])),
        v4_muls(mat.v[2], rotate.cols[2][2]));
    result.v[3] = mat.v[3];
    return result;
}

/*
 * Quat functions
 */

// TESTED: compatible with glm
MT_MATH_INLINE float quat_dot(Quat left, Quat right)
{
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z) + (left.w * right.w);
}

// TESTED: compatible with glm
MT_MATH_INLINE Quat quat_normalize(Quat left)
{
    float length = sqrtf(quat_dot(left, left));
    if (length <= 0.0f)
    {
        return (Quat){{0.0f, 0.0f, 0.0f, 1.0f}};
    }
    float one_over_length = 1.0f / length;
    return (Quat){{left.x * one_over_length,
                   left.y * one_over_length,
                   left.z * one_over_length,
                   left.w * one_over_length}};
}

// TESTED: compatible with glm
MT_MATH_INLINE Quat quat_from_axis_angle(Vec3 axis, float angle)
{
    float s = sinf(angle / 2.0f);
    Quat result;
    result.x = axis.x * s;
    result.y = axis.y * s;
    result.z = axis.z * s;
    result.w = cosf(angle / 2.0f);
    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE void quat_to_axis_angle(Quat quat, Vec3 *axis, float *angle)
{
    quat = quat_normalize(quat);
    *angle = 2.0f * acosf(quat.w);
    float s = sqrtf(1.0f - quat.w * quat.w);
    if (s < 0.001)
    {
        axis->x = quat.x;
        axis->y = quat.y;
        axis->z = quat.z;
    }
    else
    {
        axis->x = quat.x / s;
        axis->y = quat.y / s;
        axis->z = quat.z / s;
    }
}

// TESTED: compatible with glm
MT_MATH_INLINE Quat quat_conjugate(Quat q)
{
    Quat result;
    result.w = q.w;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Mat4 quat_to_mat4(Quat quat)
{
    Mat4 result = mat4_identity();

    float xx = quat.x * quat.x;
    float yy = quat.y * quat.y;
    float zz = quat.z * quat.z;
    float xy = quat.x * quat.y;
    float xz = quat.x * quat.z;
    float yz = quat.y * quat.z;
    float wx = quat.w * quat.x;
    float wy = quat.w * quat.y;
    float wz = quat.w * quat.z;

    result.cols[0][0] = 1.0f - 2.0f * (yy + zz);
    result.cols[0][1] = 2.0f * (xy + wz);
    result.cols[0][2] = 2.0f * (xz - wy);

    result.cols[1][0] = 2.0f * (xy - wz);
    result.cols[1][1] = 1.0f - 2.0f * (xx + zz);
    result.cols[1][2] = 2.0f * (yz + wx);

    result.cols[2][0] = 2.0f * (xz + wy);
    result.cols[2][1] = 2.0f * (yz - wx);
    result.cols[2][2] = 1.0f - 2.0f * (xx + yy);

    return result;
}

// TESTED: compatible with glm
MT_MATH_INLINE Quat quat_look_at(Vec3 direction, Vec3 up)
{
    float m[3][3] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
    };

    Vec3 col2 = v3_muls(direction, -1.0f);
    m[2][0] = col2.x;
    m[2][1] = col2.y;
    m[2][2] = col2.z;

    Vec3 col0 = v3_normalize(v3_cross(up, col2));
    m[0][0] = col0.x;
    m[0][1] = col0.y;
    m[0][2] = col0.z;

    Vec3 col1 = v3_cross(col2, col0);
    m[1][0] = col1.x;
    m[1][1] = col1.y;
    m[1][2] = col1.z;

    float x = m[0][0] - m[1][1] - m[2][2];
    float y = m[1][1] - m[0][0] - m[2][2];
    float z = m[2][2] - m[0][0] - m[1][1];
    float w = m[0][0] + m[1][1] + m[2][2];

    int biggest_index = 0;
    float biggest = w;
    if (x > biggest)
    {
        biggest = x;
        biggest_index = 1;
    }
    if (y > biggest)
    {
        biggest = y;
        biggest_index = 2;
    }
    if (z > biggest)
    {
        biggest = z;
        biggest_index = 3;
    }

    float biggest_val = sqrtf(biggest + 1.0f) * 0.5f;
    float mult = 0.25f / biggest_val;

    switch (biggest_index)
    {
        case 0:
            return (Quat){{
                (m[1][2] - m[2][1]) * mult,
                (m[2][0] - m[0][2]) * mult,
                (m[0][1] - m[1][0]) * mult,
                biggest_val,
            }};
        case 1:
            return (Quat){{
                biggest_val,
                (m[0][1] + m[1][0]) * mult,
                (m[2][0] + m[0][2]) * mult,
                (m[1][2] - m[2][1]) * mult,
            }};
        case 2:
            return (Quat){{
                (m[0][1] + m[1][0]) * mult,
                biggest_val,
                (m[1][2] + m[2][1]) * mult,
                (m[2][0] - m[0][2]) * mult,
            }};
        case 3:
            return (Quat){{
                (m[2][0] + m[0][2]) * mult,
                (m[1][2] + m[2][1]) * mult,
                biggest_val,
                (m[0][1] - m[1][0]) * mult,
            }};
        default: break;
    }

    return (Quat){0};
}

/*
 * Ray functions
 */

MT_MATH_INLINE float ray_distance(Ray *l1, Ray *l2)
{
    const Vec3 dp = v3_sub(l2->origin, l1->origin);
    const float v12 = v3_dot(l1->direction, l1->direction);
    const float v22 = v3_dot(l2->direction, l2->direction);
    const float v1v2 = v3_dot(l1->direction, l2->direction);

    const float det = v1v2 * v1v2 - v12 * v22;

    if (fabs(det) > FLT_MIN)
    {
        const float inv_det = 1.f / det;

        const float dpv1 = v3_dot(dp, l1->direction);
        const float dpv2 = v3_dot(dp, l2->direction);

        l1->t = inv_det * (v22 * dpv1 - v1v2 * dpv2);
        l2->t = inv_det * (v1v2 * dpv1 - v12 * dpv2);

        return v3_mag(
            v3_sub(dp, v3_sub(v3_muls(l2->direction, l2->t), v3_muls(l1->direction, l1->t))));
    }
    else
    {
        const Vec3 a = v3_cross(dp, l1->direction);
        return sqrtf(v3_dot(a, a) / v12);
    }
}

/*
 * misc functions
 */

MT_MATH_INLINE Vec3 v3_lerp(Vec3 v1, Vec3 Vec2, float t)
{
    return v3_add(v1, v3_muls(v3_sub(Vec2, v1), t));
}

MT_MATH_INLINE float lerp(float v1, float Vec2, float t)
{
    return (1 - t) * v1 + t * Vec2;
}

MT_MATH_INLINE float remap(float n, float start1, float stop1, float start2, float stop2)
{
    return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
}

MT_MATH_INLINE float clamp(float value, float min_val, float max_val)
{
    return fminf(fmaxf(value, min_val), max_val);
}

#ifdef __cplusplus
}
#endif
