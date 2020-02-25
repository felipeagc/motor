#include <motor/engine/inspector.h>

#include <float.h>
#include <stdio.h>
#include <motor/base/math.h>
#include <motor/graphics/window.h>
#include <motor/engine/entities.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/transform.h>

void mt_inspect_archetype(MtWindow *window, MtEntityArchetype *archetype)
{
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

            igText(comp_spec->name);

            switch (comp_spec->type)
            {
                case MT_COMPONENT_TYPE_TRANSFORM: {
                    MtTransform *transforms = (MtTransform *)archetype->components[c];
                    MtTransform *transform = &transforms[archetype->selected_entity];
                    igDragFloat3("Position", transform->pos.v, 1.0f, 0.0f, 0.0f, "%.3f", 1.0);
                    igDragFloat4("Rotation", transform->rot.v, 1.0f, 0.0f, 0.0f, "%.3f", 1.0);
                    igDragFloat3("Scale", transform->scale.v, 1.0f, 0.0f, 0.0f, "%.3f", 1.0);
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
                case MT_COMPONENT_TYPE_UNKNOWN: {
                    break;
                }
            }

            igPopID();
        }
    }
    igEnd();

    igPopID();
}
