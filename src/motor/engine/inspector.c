#include <motor/engine/inspector.h>

#include <float.h>
#include <stdio.h>
#include <motor/base/math.h>
#include <motor/graphics/window.h>
#include <motor/engine/entities.h>
#include <motor/engine/nuklear.h>

void mt_inspect_archetype(MtWindow *window, struct nk_context *nk, MtEntityArchetype *archetype)
{
    uint32_t width, height;
    mt_window.get_size(window, &width, &height);

    uint32_t window_width = 320;

    struct nk_rect entities_bounds = nk_rect(width - window_width, 0, window_width, height / 2);
    struct nk_rect components_bounds =
        nk_rect(width - window_width, height / 2, window_width, height / 2);

    nk_flags window_flags = NK_WINDOW_TITLE;

    if (nk_begin(nk, "Entities", entities_bounds, window_flags))
    {
        nk_layout_row_dynamic(nk, 0, 1);
        for (MtEntity e = 0; e < archetype->entity_count; ++e)
        {
            char buf[256];
            sprintf(buf, "Entity %u", e);

            int selected = archetype->selected_entity == e;
            if (nk_selectable_label(nk, buf, NK_TEXT_ALIGN_LEFT, &selected))
            {
                archetype->selected_entity = e;

                if (!selected)
                {
                    archetype->selected_entity = MT_ENTITY_INVALID;
                }
            }
        }
    }
    nk_end(nk);

    if (nk_begin(nk, "Components", components_bounds, window_flags))
    {
        for (uint32_t c = 0; c < archetype->spec.component_count; ++c)
        {
            MtComponentSpec *comp_spec = &archetype->spec.components[c];

            if (archetype->selected_entity == MT_ENTITY_INVALID)
            {
                break;
            }

            nk_layout_row_dynamic(nk, 0, 1);
            nk_label(nk, comp_spec->name, NK_TEXT_ALIGN_LEFT);

            switch (comp_spec->type)
            {
                case MT_COMPONENT_TYPE_VEC3:
                {
                    nk_layout_row_dynamic(nk, 0, 3);

                    Vec3 *vecs = (Vec3 *)archetype->components[c];
                    Vec3 *vec = &vecs[archetype->selected_entity];

                    nk_property_float(nk, "#X", -FLT_MAX, &vec->x, FLT_MAX, 0.1f, 0.0f);
                    nk_property_float(nk, "#Y", -FLT_MAX, &vec->y, FLT_MAX, 0.1f, 0.0f);
                    nk_property_float(nk, "#Z", -FLT_MAX, &vec->z, FLT_MAX, 0.1f, 0.0f);
                    break;
                }
                case MT_COMPONENT_TYPE_QUAT:
                {
                    nk_layout_row_dynamic(nk, 0, 4);

                    Quat *quats = (Quat *)archetype->components[c];
                    Quat *quat = &quats[archetype->selected_entity];

                    nk_property_float(nk, "#X", -FLT_MAX, &quat->x, FLT_MAX, 0.1f, 0.0f);
                    nk_property_float(nk, "#Y", -FLT_MAX, &quat->y, FLT_MAX, 0.1f, 0.0f);
                    nk_property_float(nk, "#Z", -FLT_MAX, &quat->z, FLT_MAX, 0.1f, 0.0f);
                    nk_property_float(nk, "#W", -FLT_MAX, &quat->w, FLT_MAX, 0.1f, 0.0f);
                    break;
                }
                case MT_COMPONENT_TYPE_UNKNOWN:
                {
                    break;
                }
            }
        }
    }
    nk_end(nk);
}
