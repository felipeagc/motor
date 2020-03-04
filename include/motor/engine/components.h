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

enum {
    MT_COMPONENT_TYPE_UNKNOWN = 0,
    MT_COMPONENT_TYPE_TRANSFORM,
    MT_COMPONENT_TYPE_POINT_LIGHT,
    MT_COMPONENT_TYPE_RIGID_ACTOR,
    MT_COMPONENT_TYPE_GLTF_MODEL,
};

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
