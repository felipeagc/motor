#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef struct MtPhysics MtPhysics;
typedef struct MtPhysicsScene MtPhysicsScene;
typedef struct MtRigidActor MtRigidActor;
typedef struct MtPhysicsShape MtPhysicsShape;

typedef uint32_t MtRigidActorType;
enum {
    MT_RIGID_ACTOR_STATIC,
    MT_RIGID_ACTOR_DYNAMIC,
};

typedef uint32_t MtPhysicsShapeType;
enum {
    MT_PHYSICS_SHAPE_SPHERE,
    MT_PHYSICS_SHAPE_PLANE,
};

typedef struct MtPhysicsTransform
{
    Vec3 pos;
    Quat rot;
} MtPhysicsTransform;

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

MT_ENGINE_API void mt_physics_scene_add_actor(MtPhysicsScene *scene, MtRigidActor *actor);

MT_ENGINE_API void mt_physics_scene_remove_actor(MtPhysicsScene *scene, MtRigidActor *actor);

MT_ENGINE_API void mt_physics_scene_step(MtPhysicsScene *scene, float dt);

//
// Shape
//

MT_ENGINE_API
MtPhysicsShape *mt_physics_shape_create(MtPhysics *physics, MtPhysicsShapeType type);

MT_ENGINE_API
MtPhysicsShapeType mt_physics_shape_get_type(MtPhysicsShape *shape);

MT_ENGINE_API
void mt_physics_shape_set_local_transform(
    MtPhysicsShape *shape, const MtPhysicsTransform *transform);

MT_ENGINE_API
MtPhysicsTransform mt_physics_shape_get_local_transform(MtPhysicsShape *shape);

// Only for sphere
MT_ENGINE_API
void mt_physics_shape_set_radius(MtPhysicsShape *shape, float radius);

// Only for sphere
MT_ENGINE_API
float mt_physics_shape_get_radius(MtPhysicsShape *shape);

//
// RigidActor
//

MT_ENGINE_API
MtRigidActor *mt_rigid_actor_create(MtPhysics *physics, MtRigidActorType type);

MT_ENGINE_API void mt_rigid_actor_destroy(MtRigidActor *actor);

MT_ENGINE_API MtPhysicsScene* mt_rigid_actor_get_scene(MtRigidActor *actor);

MT_ENGINE_API
MtRigidActorType mt_rigid_actor_get_type(MtRigidActor *actor);

MT_ENGINE_API
void mt_rigid_actor_attach_shape(MtRigidActor *actor, MtPhysicsShape *shape);

MT_ENGINE_API
void mt_rigid_actor_detach_shape(MtRigidActor *actor, MtPhysicsShape *shape);

MT_ENGINE_API
void mt_rigid_actor_get_shapes(
    MtRigidActor *actor, MtPhysicsShape **shapes, uint32_t count, uint32_t start);

MT_ENGINE_API
uint32_t mt_rigid_actor_get_shape_count(MtRigidActor *actor);

MT_ENGINE_API
MtPhysicsTransform mt_rigid_actor_get_transform(MtRigidActor *actor);

MT_ENGINE_API
void mt_rigid_actor_set_transform(MtRigidActor *actor, const MtPhysicsTransform *transform);

#ifdef __cplusplus
}
#endif
