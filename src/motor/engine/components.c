#include <motor/engine/components.h>

#include <assert.h>
#include <motor/base/intrin.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/base/buffer_writer.h>
#include <motor/engine/serializer.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/physics.h>
#include <motor/engine/engine.h>
#include <motor/engine/scene.h>

#define CHECK(exp)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        if (!(exp))                                                                                \
        {                                                                                          \
            mt_log_error("Serialization error");                                                   \
            abort();                                                                               \
        }                                                                                          \
    } while (0)

static MtComponentSpec default_component_specs[] = {
    {"Transform", sizeof(MtTransform), MT_COMPONENT_TYPE_TRANSFORM},
    {"GLTF Model", sizeof(MtGltfAsset *), MT_COMPONENT_TYPE_GLTF_MODEL},
    {"Rigid actor", sizeof(MtRigidActor *), MT_COMPONENT_TYPE_RIGID_ACTOR},
    {"Point light", sizeof(MtPointLightComponent), MT_COMPONENT_TYPE_POINT_LIGHT},
};

static void default_entity_init(MtEntityManager *em, MtEntity e)
{
    MtDefaultComponents *comps = (MtDefaultComponents *)em->components;
    comps->transform[e].pos = V3(0.f, 0.f, 0.f);
    comps->transform[e].scale = V3(1.f, 1.f, 1.f);
    comps->transform[e].rot = (Quat){0, 0, 0, 1};
    comps->model[e] = NULL;
    comps->actor[e] = NULL;
    comps->point_light[e].color = V3(1, 1, 1);
    comps->point_light[e].radius = 0.0f;
}

static void default_entity_uninit(MtEntityManager *em, MtEntity e)
{
    MtDefaultComponents *comps = (MtDefaultComponents *)em->components;
    MtPhysicsScene *physics_scene = em->scene->physics_scene;

    if (comps->actor[e] && ((em->masks[e] & MT_COMP_BIT(MtDefaultComponents, actor)) ==
                            MT_COMP_BIT(MtDefaultComponents, actor)))
    {
        mt_physics_scene_remove_actor(physics_scene, comps->actor[e]);
        mt_rigid_actor_destroy(comps->actor[e]);
    }
}

static void default_entity_serialize(MtEntityManager *em, MtBufferWriter *bw)
{
    MtDefaultComponents *comps = (MtDefaultComponents *)em->components;

    mt_serialize_array(bw, em->entity_count);

    for (MtEntity e = 0; e < em->entity_count; ++e)
    {
        assert(em->masks[e] != 0);

        uint32_t comp_count = mt_popcount64(em->masks[e]);
        mt_serialize_map(bw, comp_count);

        for (uint32_t c = 0; c < em->component_spec_count; ++c)
        {
            if ((em->masks[e] & (1 << c)) != (1 << c)) continue;

            MtComponentType comp_type = em->component_specs[c].type;
            mt_serialize_uint32(bw, comp_type); // key

            switch (comp_type)
            {
                case MT_COMPONENT_TYPE_TRANSFORM: {
                    MtTransform transform = comps->transform[e];

                    mt_serialize_map(bw, 3); // value

                    mt_serialize_string(bw, "pos");
                    mt_serialize_vec3(bw, &transform.pos);
                    mt_serialize_string(bw, "scale");
                    mt_serialize_vec3(bw, &transform.scale);
                    mt_serialize_string(bw, "rot");
                    mt_serialize_quat(bw, &transform.rot);
                    break;
                }
                case MT_COMPONENT_TYPE_GLTF_MODEL: {
                    MtAsset *asset = (MtAsset *)comps->model[e];

                    mt_serialize_map(bw, 1); // value

                    mt_serialize_string(bw, "path");
                    mt_serialize_string(bw, asset->path);
                    break;
                }
                case MT_COMPONENT_TYPE_RIGID_ACTOR: {
                    MtRigidActor *actor = comps->actor[e];
                    uint32_t shape_count = mt_rigid_actor_get_shape_count(actor);
                    MtPhysicsShape *shapes[32];
                    mt_rigid_actor_get_shapes(actor, shapes, shape_count, 0);

                    mt_serialize_map(bw, 2); // value

                    mt_serialize_string(bw, "type");
                    mt_serialize_uint32(bw, mt_rigid_actor_get_type(actor));

                    mt_serialize_string(bw, "shapes");
                    mt_serialize_array(bw, shape_count);
                    for (uint32_t i = 0; i < shape_count; ++i)
                    {
                        MtPhysicsShape *shape = shapes[i];

                        MtPhysicsShapeType shape_type = mt_physics_shape_get_type(shape);
                        MtPhysicsTransform shape_transform =
                            mt_physics_shape_get_local_transform(shape);

                        switch (shape_type)
                        {
                            case MT_PHYSICS_SHAPE_SPHERE: {
                                mt_serialize_map(bw, 4);

                                mt_serialize_string(bw, "type");
                                mt_serialize_uint32(bw, mt_physics_shape_get_type(shape));

                                mt_serialize_string(bw, "radius");
                                mt_serialize_float32(bw, mt_physics_shape_get_type(shape));

                                mt_serialize_string(bw, "pos");
                                mt_serialize_vec3(bw, &shape_transform.pos);

                                mt_serialize_string(bw, "rot");
                                mt_serialize_quat(bw, &shape_transform.rot);
                                break;
                            }
                            case MT_PHYSICS_SHAPE_PLANE: {
                                mt_serialize_map(bw, 3);

                                mt_serialize_string(bw, "type");
                                mt_serialize_uint32(bw, mt_physics_shape_get_type(shape));

                                mt_serialize_string(bw, "pos");
                                mt_serialize_vec3(bw, &shape_transform.pos);

                                mt_serialize_string(bw, "rot");
                                mt_serialize_quat(bw, &shape_transform.rot);
                                break;
                            }
                            default: assert(0); break;
                        }
                    }

                    break;
                }
                case MT_COMPONENT_TYPE_POINT_LIGHT: {
                    MtPointLightComponent point_light = comps->point_light[e];

                    mt_serialize_map(bw, 2); // value

                    mt_serialize_string(bw, "color");
                    mt_serialize_vec3(bw, &point_light.color);
                    mt_serialize_string(bw, "radius");
                    mt_serialize_float32(bw, point_light.radius);
                    break;
                }
                default: assert(0); break;
            }
        }
    }
}

static void default_entity_deserialize(MtEntityManager *em, MtBufferReader *br)
{
    MtAssetManager *am = em->scene->asset_manager;
    MtDefaultComponents *comps = (MtDefaultComponents *)em->components;

    MtSerializeValue array_value = {0};

    CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_ARRAY, &array_value));
    for (uint32_t i = 0; i < array_value.array.element_count; ++i)
    {
        MtEntity e = mt_entity_manager_add_entity(em, 0);

        MtSerializeValue map_value = {0};
        CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_MAP, &map_value));

        MtSerializeValue comp_key = {0};
        MtSerializeValue comp_value = {0};

        for (uint32_t j = 0; j < map_value.map.pair_count; ++j)
        {
            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_UINT32, &comp_key)); // key
            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_MAP, &comp_value));  // value

            MtSerializeValue field_key = {0};
            MtSerializeValue field_value = {0};

            switch (comp_key.uint32)
            {
                case MT_COMPONENT_TYPE_TRANSFORM: {
                    em->masks[e] |= MT_COMP_BIT(MtDefaultComponents, transform);
                    for (uint32_t k = 0; k < comp_value.map.pair_count; ++k)
                    {
                        CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_STRING, &field_key));
                        if (strncmp("pos", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_VEC3, &field_value));
                            comps->transform[e].pos = field_value.vec3;
                        }
                        if (strncmp("scale", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_VEC3, &field_value));
                            comps->transform[e].scale = field_value.vec3;
                        }
                        if (strncmp("rot", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_QUAT, &field_value));
                            comps->transform[e].rot = field_value.quat;
                        }
                    }
                    break;
                }
                case MT_COMPONENT_TYPE_GLTF_MODEL: {
                    em->masks[e] |= MT_COMP_BIT(MtDefaultComponents, model);
                    for (uint32_t k = 0; k < comp_value.map.pair_count; ++k)
                    {
                        CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_STRING, &field_key));
                        if (strncmp("path", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_STRING, &field_value));
                            comps->model[e] =
                                (MtGltfAsset *)mt_asset_manager_get(am, field_value.str.buf);
                        }
                    }
                    break;
                }
                case MT_COMPONENT_TYPE_RIGID_ACTOR: {
                    em->masks[e] |= MT_COMP_BIT(MtDefaultComponents, actor);

                    MtSerializeValue actor_type = {0};

                    /*array*/ MtPhysicsShape **shapes = NULL;

                    for (uint32_t k = 0; k < comp_value.map.pair_count; ++k)
                    {
                        CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_STRING, &field_key));
                        if (strncmp("type", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_UINT32, &actor_type));
                        }
                        if (strncmp("shapes", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_ARRAY, &field_value));
                            for (uint32_t s = 0; s < field_value.array.element_count; ++s)
                            {
                                MtSerializeValue shape_type = {0};
                                MtSerializeValue shape_radius = {0};
                                MtSerializeValue shape_pos = {0};
                                MtSerializeValue shape_rot = {0};

                                MtSerializeValue shape_prop_map = {0};
                                CHECK(mt_deserialize_value(
                                    br, MT_SERIALIZE_TYPE_MAP, &shape_prop_map));

                                MtSerializeValue shape_prop_key = {0};

                                for (uint32_t p = 0; p < shape_prop_map.array.element_count; ++p)
                                {
                                    CHECK(mt_deserialize_value(
                                        br, MT_SERIALIZE_TYPE_STRING, &shape_prop_key));

                                    if (strncmp(
                                            "type",
                                            shape_prop_key.str.buf,
                                            shape_prop_key.str.length) == 0)
                                    {
                                        CHECK(mt_deserialize_value(
                                            br, MT_SERIALIZE_TYPE_UINT32, &shape_type));
                                    }

                                    if (strncmp(
                                            "radius",
                                            shape_prop_key.str.buf,
                                            shape_prop_key.str.length) == 0)
                                    {
                                        CHECK(mt_deserialize_value(
                                            br, MT_SERIALIZE_TYPE_FLOAT32, &shape_radius));
                                    }

                                    if (strncmp(
                                            "pos",
                                            shape_prop_key.str.buf,
                                            shape_prop_key.str.length) == 0)
                                    {
                                        CHECK(mt_deserialize_value(
                                            br, MT_SERIALIZE_TYPE_VEC3, &shape_pos));
                                    }

                                    if (strncmp(
                                            "rot",
                                            shape_prop_key.str.buf,
                                            shape_prop_key.str.length) == 0)
                                    {
                                        CHECK(mt_deserialize_value(
                                            br, MT_SERIALIZE_TYPE_QUAT, &shape_rot));
                                    }
                                }

                                CHECK(shape_type.type > 0);

                                MtPhysicsShape *shape = mt_physics_shape_create(
                                    em->scene->engine->physics, shape_type.uint32);

                                if (shape_radius.type > 0 &&
                                    shape_type.type == MT_PHYSICS_SHAPE_SPHERE)
                                {
                                    mt_physics_shape_set_radius(shape, shape_radius.f32);
                                }

                                MtPhysicsTransform physics_transform = {0};

                                if (shape_pos.type > 0)
                                {
                                    physics_transform.pos = shape_pos.vec3;
                                }

                                if (shape_rot.type > 0)
                                {
                                    physics_transform.rot = shape_rot.quat;
                                }

                                mt_physics_shape_set_local_transform(shape, &physics_transform);

                                mt_array_push(NULL, shapes, shape);
                            }
                        }
                    }

                    CHECK(actor_type.type > 0);
                    comps->actor[e] =
                        mt_rigid_actor_create(em->scene->engine->physics, actor_type.uint32);
                    mt_physics_scene_add_actor(em->scene->physics_scene, comps->actor[e]);

                    for (MtPhysicsShape **shape = shapes; shape != shapes + mt_array_size(shapes);
                         ++shape)
                    {
                        mt_rigid_actor_attach_shape(comps->actor[e], *shape);
                    }

                    mt_array_free(NULL, shapes);
                    break;
                }
                case MT_COMPONENT_TYPE_POINT_LIGHT: {
                    em->masks[e] |= MT_COMP_BIT(MtDefaultComponents, point_light);
                    for (uint32_t k = 0; k < comp_value.map.pair_count; ++k)
                    {
                        CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_STRING, &field_key));
                        if (strncmp("color", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(mt_deserialize_value(br, MT_SERIALIZE_TYPE_VEC3, &field_value));
                            comps->point_light[e].color = field_value.vec3;
                        }
                        if (strncmp("radius", field_key.str.buf, field_key.str.length) == 0)
                        {
                            CHECK(
                                mt_deserialize_value(br, MT_SERIALIZE_TYPE_FLOAT32, &field_value));
                            comps->point_light[e].radius = field_value.f32;
                        }
                    }
                    break;
                }
            }
        }
    }
}

MT_ENGINE_API MtEntityDescriptor mt_default_entity_descriptor = {
    .entity_init = default_entity_init,
    .entity_uninit = default_entity_uninit,
    .entity_serialize = default_entity_serialize,
    .entity_deserialize = default_entity_deserialize,
    .component_specs = default_component_specs,
    .component_spec_count = MT_LENGTH(default_component_specs),
};
