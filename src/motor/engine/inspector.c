#include <motor/engine/inspector.h>

#include <float.h>
#include <stdio.h>
#include <motor/base/math.h>
#include <motor/graphics/window.h>
#include <motor/engine/engine.h>
#include <motor/engine/entities.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/transform.h>
#include <motor/engine/physics.h>

void mt_inspect_archetype(MtEngine *engine, MtEntityArchetype *archetype)
{
    MtWindow *window = engine->window;

    uint32_t width, height;
    mt_window.get_size(window, &width, &height);

    igPushIDPtr(archetype);

    uint32_t win_width = 320;
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    igSetNextWindowSize((ImVec2){(float)win_width, (float)height / 2.0f}, 0);
    igSetNextWindowPos((ImVec2){(float)(width - win_width), 0.0f}, 0, (ImVec2){});

    if (igBegin("Entities", NULL, window_flags))
    {
        for (MtEntity e = 0; e < archetype->entity_count; ++e)
        {
            igPushIDInt(e);

            char buf[256];
            sprintf(buf, "Entity %u", e);

            bool selected = archetype->selected_entity == e;
            if (igSelectableBoolPtr(buf, &selected, 0, (ImVec2){}))
            {
                archetype->selected_entity = e;

                if (!selected)
                {
                    archetype->selected_entity = MT_ENTITY_INVALID;
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
        for (uint32_t c = 0; c < archetype->spec.component_count; ++c)
        {
            MtComponentSpec *comp_spec = &archetype->spec.components[c];

            if (archetype->selected_entity == MT_ENTITY_INVALID)
            {
                break;
            }

            igPushIDInt(c);

            if (igCollapsingHeader(
                    comp_spec->name,
                    ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding))
            {
                switch (comp_spec->type)
                {
                    case MT_COMPONENT_TYPE_TRANSFORM: {
                        MtTransform *transforms = (MtTransform *)archetype->components[c];
                        MtTransform *transform = &transforms[archetype->selected_entity];
                        igDragFloat3("Position", transform->pos.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                        igDragFloat4("Rotation", transform->rot.v, 0.1f, -1.0f, 1.0f, "%.3f", 1.0);
                        igDragFloat3("Scale", transform->scale.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                        break;
                    }
                    case MT_COMPONENT_TYPE_VEC3: {
                        Vec3 *vecs = (Vec3 *)archetype->components[c];
                        Vec3 *vec = &vecs[archetype->selected_entity];
                        igDragFloat3("", vec->v, 1.0f, 0.0f, 0.0f, "%.3f", 1.0);
                        break;
                    }
                    case MT_COMPONENT_TYPE_QUAT: {
                        Quat *quats = (Quat *)archetype->components[c];
                        Quat *quat = &quats[archetype->selected_entity];
                        igDragFloat4("", quat->v, 1.0f, 0.0f, 0.0f, "%.3f", 1.0);
                        break;
                    }
                    case MT_COMPONENT_TYPE_RIGID_ACTOR: {
                        MtRigidActor **actors = (MtRigidActor **)archetype->components[c];
                        MtRigidActor *actor = actors[archetype->selected_entity];

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

                            MtPhysicsTransform transform =
                                mt_physics_shape_get_local_transform(shape);
                            igDragFloat3(
                                "Position", transform.pos.v, 0.1f, 0.0f, 0.0f, "%.3f", 1.0);
                            igDragFloat4(
                                "Rotation", transform.rot.v, 0.1f, -1.0f, 1.0f, "%.3f", 1.0);
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
                                MtPhysicsShape *shape =
                                    mt_physics_shape_create(engine->physics, type);
                                mt_rigid_actor_attach_shape(actor, shape);
                                type = 0;
                                igCloseCurrentPopup();
                            }
                            igEndPopup();
                        }

                        break;
                    }
                    case MT_COMPONENT_TYPE_GLTF_MODEL: {
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
    igEnd();

    igPopID();
}
