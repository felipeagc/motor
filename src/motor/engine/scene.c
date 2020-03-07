#include <motor/engine/scene.h>

#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/entities.h>
#include <motor/engine/components.h>
#include <motor/engine/physics.h>

void mt_scene_init(MtScene *scene, MtEngine *engine)
{
    memset(scene, 0, sizeof(*scene));

    scene->engine = engine;

    MtAllocator *alloc = scene->engine->alloc;

    scene->asset_manager = mt_alloc(alloc, sizeof(*scene->asset_manager));
    scene->entity_manager = mt_alloc(alloc, sizeof(*scene->entity_manager));

    scene->physics_scene = mt_physics_scene_create(engine->physics);
    mt_asset_manager_init(scene->asset_manager, engine);
    mt_entity_manager_init(scene->entity_manager, alloc, scene, &mt_default_entity_descriptor);

    mt_perspective_camera_init(&scene->cam);
    mt_environment_init(&scene->env, scene->engine);

    scene->graph = mt_render.create_graph(engine->device, engine->swapchain, true);
}

void mt_scene_destroy(MtScene *scene)
{
    MtAllocator *alloc = scene->engine->alloc;

    mt_render.destroy_graph(scene->graph);

    mt_environment_destroy(&scene->env);

    mt_entity_manager_destroy(scene->entity_manager);
    mt_asset_manager_destroy(scene->asset_manager);
    mt_physics_scene_destroy(scene->physics_scene);

    mt_free(alloc, scene->entity_manager);
    mt_free(alloc, scene->asset_manager);
}
