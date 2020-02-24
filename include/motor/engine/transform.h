#pragma once

#include <motor/base/math_types.h>
#include <motor/base/math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtTransform
{
    Vec3 pos;
    Vec3 scale;
    Quat rot;
} MtTransform;

static inline Mat4 mt_transform_matrix(const MtTransform *transform)
{
    Mat4 mat = mat4_identity();
    mat = mat4_scale(mat, transform->scale);
    mat = mat4_mul(quat_to_mat4(transform->rot), mat);
    mat = mat4_translate(mat, transform->pos);
    return mat;
}

#ifdef __cplusplus
}
#endif
