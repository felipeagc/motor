#include "../../include/motor/asset_manager.h"

#include <string.h>
#include "../../include/motor/allocator.h"
#include "../../include/motor/array.h"
#include "../../include/motor/engine.h"

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine) {
    memset(am, 0, sizeof(*am));
    am->engine = engine;
    am->alloc  = &engine->alloc;
}

MtIAsset *mt_asset_manager_add(MtAssetManager *am, MtIAsset asset) {
    return mt_array_push(am->alloc, am->assets, asset);
}

void mt_asset_manager_destroy(MtAssetManager *am) {
    for (uint32_t i = 0; i < mt_array_size(am->assets); i++) {
        MtIAsset *asset = &am->assets[i];
        asset->vt->destroy(asset->inst);
    }
    mt_array_free(am->alloc, am->assets);
}
