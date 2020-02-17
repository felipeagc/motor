#include <motor/engine/physics.h>

#include <new>
#include <assert.h>
#include <PxPhysicsAPI.h>
#include <motor/base/allocator.h>
#include <motor/base/log.h>

using namespace physx;

struct MtPhysics
{
    MtAllocator *alloc;

    PxDefaultAllocator allocator;
    PxDefaultErrorCallback error_callback;
    PxFoundation *foundation;
    PxPhysics *physics;
};

struct MtPhysicsScene
{
    MtPhysics *physics;
    PxDefaultCpuDispatcher *dispatcher;
    PxScene *scene;

    float accumulator;
    float step_size;
};

struct RigidActor
{
    PxRigidActor *actor;
};

static_assert(sizeof(RigidActor) == sizeof(MtRigidActor), "");

//
// Physics
//

extern "C" MtPhysics *mt_physics_create(MtAllocator *alloc)
{
    MtPhysics *physics = new (mt_alloc(alloc, sizeof(*physics))) MtPhysics;

    physics->alloc = alloc;

    physics->foundation =
        PxCreateFoundation(PX_PHYSICS_VERSION, physics->allocator, physics->error_callback);

    physics->physics =
        PxCreatePhysics(PX_PHYSICS_VERSION, *physics->foundation, PxTolerancesScale(), true, NULL);

    return physics;
}

extern "C" void mt_physics_destroy(MtPhysics *physics)
{
    MtAllocator *alloc = physics->alloc;

    physics->physics->release();
    physics->foundation->release();

    mt_free(alloc, physics);
}

//
// Scene
//

extern "C" MtPhysicsScene *mt_physics_scene_create(MtPhysics *physics)
{
    MtAllocator *alloc = physics->alloc;
    MtPhysicsScene *scene = new (mt_alloc(alloc, sizeof(*scene))) MtPhysicsScene;

    scene->accumulator = 0.0f;
    scene->step_size = 1.0f / 60.0f;

    scene->physics = physics;

    scene->dispatcher = PxDefaultCpuDispatcherCreate(2);

    PxSceneDesc scene_desc(physics->physics->getTolerancesScale());
    scene_desc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    scene_desc.cpuDispatcher = scene->dispatcher;
    scene_desc.filterShader = PxDefaultSimulationFilterShader;
    scene->scene = physics->physics->createScene(scene_desc);

    return scene;
}

extern "C" void mt_physics_scene_destroy(MtPhysicsScene *scene)
{
    MtAllocator *alloc = scene->physics->alloc;

    scene->dispatcher->release();
    scene->scene->release();

    mt_free(alloc, scene);
}

extern "C" void mt_physics_scene_step(MtPhysicsScene *scene, float dt)
{
    scene->accumulator += dt;

    if (scene->accumulator < scene->step_size)
    {
        return;
    }

    scene->accumulator -= scene->step_size;

    scene->scene->simulate(scene->step_size);
    scene->scene->fetchResults(true);
}

//
// RigidActor
//

extern "C" void mt_rigid_actor_init(
    MtPhysicsScene *scene,
    MtRigidActor *actor_,
    MtRigidActorType type,
    MtPhysicsShape *shape,
    Vec3 pos,
    Quat rot)
{
    MtPhysics *physics = scene->physics;

    PxMaterial *material = physics->physics->createMaterial(0.5f, 0.5f, 0.6f);
    PxShape *shape_ = NULL;

    PxTransform transform;
    transform.q = PxQuat(rot.x, rot.y, rot.z, rot.w);
    transform.p = PxVec3(pos.x, pos.y, pos.z);

    switch (shape->type)
    {
        case MT_PHYSICS_SHAPE_SPHERE: {
            shape_ = physics->physics->createShape(PxSphereGeometry(shape->radius), *material);
            break;
        }
        case MT_PHYSICS_SHAPE_PLANE: {
            assert(type == MT_RIGID_ACTOR_STATIC);
            shape_ = physics->physics->createShape(PxPlaneGeometry{}, *material);
            shape_->setLocalPose(PxTransformFromPlaneEquation(
                PxPlane(shape->plane.x, shape->plane.y, shape->plane.z, shape->plane.w)));
            break;
        }
    }

    assert(shape_ != NULL);

    RigidActor actor = {};

    switch (type)
    {
        case MT_RIGID_ACTOR_DYNAMIC: {
            actor.actor = physics->physics->createRigidDynamic(transform);
            break;
        }
        case MT_RIGID_ACTOR_STATIC: {
            actor.actor = physics->physics->createRigidStatic(transform);
            break;
        }
    }

    actor.actor->attachShape(*shape_);
    shape_->release();
    material->release();

    scene->scene->addActor(*actor.actor);

    memcpy(actor_, &actor, sizeof(actor));
}

extern "C" void mt_rigid_actor_get_transform(MtRigidActor *actor_, Vec3 *pos, Quat *rot)
{
    RigidActor actor = {};
    memcpy(&actor, actor_, sizeof(actor));
    PxTransform transform = actor.actor->getGlobalPose();

    pos->x = transform.p.x;
    pos->y = transform.p.y;
    pos->z = transform.p.z;

    rot->x = transform.q.x;
    rot->y = transform.q.y;
    rot->z = transform.q.z;
    rot->w = transform.q.w;
}

extern "C" void mt_rigid_actor_set_transform(MtRigidActor *actor_, Vec3 pos, Quat rot)
{
    RigidActor actor = {};
    memcpy(&actor, actor_, sizeof(actor));

    PxTransform transform;
    transform.p.x = pos.x;
    transform.p.y = pos.y;
    transform.p.z = pos.z;

    transform.q.x = rot.x;
    transform.q.y = rot.y;
    transform.q.z = rot.z;
    transform.q.w = rot.w;

    actor.actor->setGlobalPose(transform);
}
