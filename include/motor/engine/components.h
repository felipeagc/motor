#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>
#include <motor/engine/entities.h>
#include <motor/engine/transform.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtGltfAsset MtGltfAsset;
typedef struct MtRigidActor MtRigidActor;

typedef struct MtPointLightComponent
{
    Vec3 color;
    float radius;
} MtPointLightComponent;

typedef struct MtDefaultComponents
{
    MtTransform *transform;
    MtGltfAsset **model;
    MtRigidActor **actor;
    MtPointLightComponent *point_light;
} MtDefaultComponents;

MT_ENGINE_API extern MtEntityDescriptor mt_default_entity_descriptor;

#ifdef __cplusplus
}
#endif
