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
// Shape
//

extern "C" MtPhysicsShape *mt_physics_shape_create(MtPhysics *physics, MtPhysicsShapeType type)
{
    PxShape *shape = NULL;
    PxMaterial *material = physics->physics->createMaterial(0.5f, 0.5f, 0.6f);

    switch (type)
    {
        case MT_PHYSICS_SHAPE_SPHERE: {
            shape = physics->physics->createShape(PxSphereGeometry{}, *material);
            break;
        }
        case MT_PHYSICS_SHAPE_PLANE: {
            shape = physics->physics->createShape(PxPlaneGeometry{}, *material);
            break;
        }
    }

    material->release();

    return (MtPhysicsShape *)shape;
}

extern "C" MtPhysicsShapeType mt_physics_shape_get_type(MtPhysicsShape *shape_)
{
    PxShape *shape = (PxShape *)shape_;
    switch (shape->getGeometryType())
    {
        case PxGeometryType::eSPHERE: return MT_PHYSICS_SHAPE_SPHERE;
        case PxGeometryType::ePLANE: return MT_PHYSICS_SHAPE_PLANE;
        default: assert(0);
    }
}

extern "C" void
mt_physics_shape_set_local_transform(MtPhysicsShape *shape_, const MtPhysicsTransform *transform)
{
    PxShape *shape = (PxShape *)shape_;
    shape->setLocalPose(PxTransform{
        PxVec3{transform->pos.x, transform->pos.y, transform->pos.z},
        PxQuat{transform->rot.x, transform->rot.y, transform->rot.z, transform->rot.w}});
}

extern "C" MtPhysicsTransform mt_physics_shape_get_local_transform(MtPhysicsShape *shape_)
{
    PxShape *shape = (PxShape *)shape_;

    PxTransform px_transform = shape->getLocalPose();
    MtPhysicsTransform transform;

    transform.pos.x = px_transform.p.x;
    transform.pos.y = px_transform.p.y;
    transform.pos.z = px_transform.p.z;

    transform.rot.x = px_transform.q.x;
    transform.rot.y = px_transform.q.y;
    transform.rot.z = px_transform.q.z;
    transform.rot.w = px_transform.q.w;

    return transform;
}

extern "C" void mt_physics_shape_set_radius(MtPhysicsShape *shape_, float radius)
{
    PxShape *shape = (PxShape *)shape_;
    shape->setGeometry(PxSphereGeometry(radius));
}

extern "C" float mt_physics_shape_get_radius(MtPhysicsShape *shape_)
{
    PxShape *shape = (PxShape *)shape_;
    PxSphereGeometry geom;
    shape->getSphereGeometry(geom);
    return geom.radius;
}

//
// RigidActor
//

extern "C" MtRigidActor *mt_rigid_actor_create(MtPhysicsScene *scene, MtRigidActorType type)
{
    MtPhysics *physics = scene->physics;

    PxRigidActor *actor = NULL;

    switch (type)
    {
        case MT_RIGID_ACTOR_DYNAMIC: {
            actor = physics->physics->createRigidDynamic({});
            break;
        }
        case MT_RIGID_ACTOR_STATIC: {
            actor = physics->physics->createRigidStatic({});
            break;
        }
    }

    assert(actor);

    scene->scene->addActor(*actor);

    return (MtRigidActor *)actor;
}

extern "C" void mt_rigid_actor_attach_shape(MtRigidActor *actor_, MtPhysicsShape *shape_)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    PxShape *shape = (PxShape *)shape_;
    actor->attachShape(*shape);
}

extern "C" void mt_rigid_actor_detach_shape(MtRigidActor *actor_, MtPhysicsShape *shape_)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    PxShape *shape = (PxShape *)shape_;
    actor->detachShape(*shape);
}

extern "C" uint32_t mt_rigid_actor_get_shape_count(MtRigidActor *actor_)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    return actor->getNbShapes();
}

extern "C" void mt_rigid_actor_get_shapes(
    MtRigidActor *actor_, MtPhysicsShape **out_shapes, uint32_t count, uint32_t start)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    actor->getShapes((PxShape **)out_shapes, count, start);
}

extern "C" MtPhysicsTransform mt_rigid_actor_get_transform(MtRigidActor *actor_)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    PxTransform px_transform = actor->getGlobalPose();
    MtPhysicsTransform transform;

    transform.pos.x = px_transform.p.x;
    transform.pos.y = px_transform.p.y;
    transform.pos.z = px_transform.p.z;

    transform.rot.x = px_transform.q.x;
    transform.rot.y = px_transform.q.y;
    transform.rot.z = px_transform.q.z;
    transform.rot.w = px_transform.q.w;

    return transform;
}

extern "C" void
mt_rigid_actor_set_transform(MtRigidActor *actor_, const MtPhysicsTransform *transform)
{
    PxRigidActor *actor = (PxRigidActor *)actor_;
    actor->setGlobalPose(PxTransform{
        PxVec3{transform->pos.x, transform->pos.y, transform->pos.z},
        PxQuat{transform->rot.x, transform->rot.y, transform->rot.z, transform->rot.w}});
}
