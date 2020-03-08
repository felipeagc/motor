#include <motor/engine/inspector.h>

#include <float.h>
#include <stdio.h>
#include <motor/base/math.h>
#include <motor/graphics/window.h>
#include <motor/engine/engine.h>
#include <motor/engine/entities.h>
#include <motor/engine/components.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/transform.h>
#include <motor/engine/physics.h>
#include <motor/engine/asset_manager.h>

static void inspect_components(MtEngine *engine, MtEntityManager *em)
{
    if (em->selected_entity == MT_ENTITY_INVALID)
    {
        return;
    }

    if (igButton("Remove entity", (ImVec2){}))
    {
        mt_entity_manager_remove_entity(em, em->selected_entity);
        return;
    }

    if (igCollapsingHeader(
            "Component masks", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding))
    {
        for (uint32_t c = 0; c < em->component_spec_count; ++c)
        {
            uint32_t comp_bit = 1 << c;
            MtComponentMask *mask = &em->masks[em->selected_entity];
            MtComponentSpec *comp_spec = &em->component_specs[c];
            uint8_t *comp_data = em->components[c] + (comp_spec->size * em->selected_entity);
            if (igCheckboxFlags(comp_spec->name, mask, comp_bit))
            {
                if ((*mask & comp_bit) == comp_bit)
                {
                    // Added the component
                    if (comp_spec->init)
                    {
                        comp_spec->init(em, comp_data);
                    }
                }
                else
                {
                    // Removed the component
                    if (comp_spec->unregister)
                    {
                        comp_spec->unregister(em, comp_data);
                    }
                }
            }
        }
    }

    for (uint32_t c = 0; c < em->component_spec_count; ++c)
    {
        MtComponentSpec *comp_spec = &em->component_specs[c];

        if ((em->masks[em->selected_entity] & (1 << c)) != (1 << c))
        {
            continue;
        }

        igPushIDInt(c);

        if (igCollapsingHeader(
                comp_spec->name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding))
        {
            switch (comp_spec->type)
            {
                case MT_COMPONENT_TYPE_TRANSFORM: {
                    MtTransform *transforms = (MtTransform *)em->components[c];
                    MtTransform *transform = &transforms[em->selected_entity];
                    igDragFloat3("Position", transform->pos.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                    igDragFloat4("Rotation", transform->rot.v, 0.1f, -1.0f, 1.0f, "%.3f", 1.0);
                    igDragFloat3("Scale", transform->scale.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                    break;
                }
                case MT_COMPONENT_TYPE_RIGID_ACTOR: {
                    MtRigidActor **actors = (MtRigidActor **)em->components[c];
                    MtRigidActor *actor = actors[em->selected_entity];

                    uint32_t shape_count = mt_rigid_actor_get_shape_count(actor);
                    MtPhysicsShape *shapes[32];
                    mt_rigid_actor_get_shapes(actor, shapes, shape_count, 0);

                    for (uint32_t i = 0; i < shape_count; ++i)
                    {
                        igPushIDInt(i);

                        MtPhysicsShape *shape = shapes[i];
                        MtPhysicsShapeType shape_type = mt_physics_shape_get_type(shape);

                        igBeginGroup();

                        switch (shape_type)
                        {
                            case MT_PHYSICS_SHAPE_SPHERE: {
                                igText("Sphere shape");
                                float radius = mt_physics_shape_get_radius(shape);
                                igDragFloat("Radius", &radius, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
                                mt_physics_shape_set_radius(shape, radius);
                                break;
                            }
                            case MT_PHYSICS_SHAPE_PLANE: {
                                igText("Plane shape");
                                break;
                            }
                        }

                        igSameLine(0.0f, -1.0f);

                        if (igButton("Remove", (ImVec2){}))
                        {
                            mt_rigid_actor_detach_shape(actor, shape);
                        }

                        igEndGroup();

                        MtPhysicsTransform transform = mt_physics_shape_get_local_transform(shape);
                        igDragFloat3("Position", transform.pos.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                        igDragFloat4("Rotation", transform.rot.v, 0.1f, -1.0f, 1.0f, "%.3f", 1.0);
                        mt_physics_shape_set_local_transform(shape, &transform);

                        igSeparator();

                        igPopID();
                    }

                    if (igButton("Add shape", (ImVec2){}))
                    {
                        igOpenPopup("add-shape");
                    }

                    if (igBeginPopup("add-shape", 0))
                    {
                        static MtPhysicsShapeType type = 0;
                        if (igRadioButtonBool("Sphere shape", type == MT_PHYSICS_SHAPE_SPHERE))
                            type = MT_PHYSICS_SHAPE_SPHERE;
                        if (igRadioButtonBool("Plane shape", type == MT_PHYSICS_SHAPE_PLANE))
                            type = MT_PHYSICS_SHAPE_PLANE;
                        if (igButton("Add", (ImVec2){}))
                        {
                            MtPhysicsShape *shape = mt_physics_shape_create(engine->physics, type);
                            mt_rigid_actor_attach_shape(actor, shape);
                            type = 0;
                            igCloseCurrentPopup();
                        }
                        igEndPopup();
                    }

                    break;
                }
                case MT_COMPONENT_TYPE_GLTF_MODEL: {
                    MtGltfAsset **gltf_assets = (MtGltfAsset **)em->components[c];
                    MtGltfAsset *gltf_asset = gltf_assets[em->selected_entity];

                    MtAsset *asset = (MtAsset *)gltf_asset;
                    igText(asset->path);
                    break;
                }
                case MT_COMPONENT_TYPE_UNKNOWN: {
                    break;
                }
            }
        }

        igPopID();
    }
}

void mt_inspect_entities(MtEngine *engine, MtEntityManager *em)
{
    MtWindow *window = engine->window;

    uint32_t width, height;
    mt_window.get_size(window, &width, &height);

    igPushIDPtr(em);

    uint32_t win_width = 320;
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    igSetNextWindowSize((ImVec2){(float)win_width, (float)height / 2.0f}, 0);
    igSetNextWindowPos((ImVec2){(float)(width - win_width), 0.0f}, 0, (ImVec2){});

    if (igBegin("Entities", NULL, window_flags))
    {
        for (MtEntity e = 0; e < em->entity_count; ++e)
        {
            igPushIDInt(e);

            char buf[256];
            sprintf(buf, "Entity %u", e);

            bool selected = em->selected_entity == e;
            if (igSelectableBoolPtr(buf, &selected, 0, (ImVec2){}))
            {
                em->selected_entity = e;

                if (!selected)
                {
                    em->selected_entity = MT_ENTITY_INVALID;
                }
            }

            igPopID();
        }
    }
    igEnd();

    igSetNextWindowSize((ImVec2){(float)win_width, (float)height / 2.0f}, 0);
    igSetNextWindowPos((ImVec2){(float)(width - win_width), (float)height / 2.0f}, 0, (ImVec2){});
    if (igBegin("Components", NULL, window_flags))
    {
        inspect_components(engine, em);
    }
    igEnd();

    igPopID();
}
