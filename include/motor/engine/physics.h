#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef struct MtPhysics MtPhysics;
typedef struct MtPhysicsScene MtPhysicsScene;

typedef struct MtRigidActor
{
    void *opaque[1];
} MtRigidActor;

typedef enum MtRigidActorType {
    MT_RIGID_ACTOR_STATIC,
    MT_RIGID_ACTOR_DYNAMIC,
} MtRigidActorType;

typedef enum MtPhysicsShapeType {
    MT_PHYSICS_SHAPE_SPHERE,
    MT_PHYSICS_SHAPE_PLANE,
} MtPhysicsShapeType;

typedef struct MtPhysicsShape
{
    MtPhysicsShapeType type;
    union
    {
        struct
        {
            float radius;
        };
        struct
        {
            Vec4 plane;
        };
    };
} MtPhysicsShape;

//
// Physics
//

MT_ENGINE_API MtPhysics *mt_physics_create(MtAllocator *alloc);

MT_ENGINE_API void mt_physics_destroy(MtPhysics *physics);

//
// Scene
//

MT_ENGINE_API MtPhysicsScene *mt_physics_scene_create(MtPhysics *physics);

MT_ENGINE_API void mt_physics_scene_destroy(MtPhysicsScene *scene);

MT_ENGINE_API void mt_physics_scene_step(MtPhysicsScene *scene, float dt);

//
// RigidActor
//

MT_ENGINE_API
void mt_rigid_actor_init(
    MtPhysicsScene *scene,
    MtRigidActor *actor,
    MtRigidActorType type,
    MtPhysicsShape *shape,
    Vec3 pos,
    Quat rot);

MT_ENGINE_API
void mt_rigid_actor_get_transform(MtRigidActor *actor, Vec3 *pos, Quat *rot);

MT_ENGINE_API
void mt_rigid_actor_set_transform(MtRigidActor *actor, Vec3 pos, Quat rot);

#ifdef __cplusplus
}
#endif
