#include <stdio.h>
#include <PxPhysicsAPI.h>

using namespace physx;

int main()
{
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;

    PxFoundation *foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

    PxPhysics *physics =
        PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, NULL);

    PxDefaultCpuDispatcher *dispatcher = PxDefaultCpuDispatcherCreate(2);

    dispatcher->release();
    physics->release();
    foundation->release();

    return 0;
}
