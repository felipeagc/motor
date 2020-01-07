#include <stdlib.h>
#include "../../base/xxhash.h"

static uint64_t vulkan_hash_render_pass(VkRenderPassCreateInfo *ci)
{
    XXH64_state_t state = {0};

    XXH64_hash_t const seed = 0; /* or any other value */
    if (XXH64_reset(&state, seed) == XXH_ERROR)
        abort();

    if (XXH64_update(&state, ci->pAttachments, ci->attachmentCount * sizeof(*ci->pAttachments)) ==
        XXH_ERROR)
    {
        abort();
    }

    for (uint32_t i = 0; i < ci->subpassCount; i++)
    {
        const VkSubpassDescription *subpass = &ci->pSubpasses[i];
        if (XXH64_update(&state, &subpass->pipelineBindPoint, sizeof(subpass->pipelineBindPoint)) ==
            XXH_ERROR)
        {
            abort();
        }

        if (XXH64_update(
                &state,
                subpass->pColorAttachments,
                subpass->colorAttachmentCount * sizeof(*subpass->pColorAttachments)) == XXH_ERROR)
        {
            abort();
        }
    }

    if (XXH64_update(&state, ci->pDependencies, ci->dependencyCount * sizeof(*ci->pDependencies)) ==
        XXH_ERROR)
    {
        abort();
    }

    return (uint64_t)XXH64_digest(&state);
}
