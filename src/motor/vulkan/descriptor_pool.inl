static void descriptor_pool_grow(MtDevice *dev, DescriptorPool *p) {
    VkDescriptorPool *pool =
        mt_array_push(dev->alloc, p->pools, (VkDescriptorPool){0});
    MtHashMap *hashmap =
        mt_array_push(dev->alloc, p->pool_hashmaps, (MtHashMap){0});
    uint32_t *allocated_set_count =
        mt_array_push(dev->alloc, p->allocated_set_counts, (uint32_t){0});

    mt_hash_init(hashmap, SETS_PER_PAGE * 2, dev->alloc);

    *allocated_set_count = 0;

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = SETS_PER_PAGE,
        .poolSizeCount = mt_array_size(p->pool_sizes),
        .pPoolSizes    = p->pool_sizes,
    };

    VK_CHECK(
        vkCreateDescriptorPool(dev->device, &pool_create_info, NULL, pool));

    // Allocate descriptor sets
    VkDescriptorSetLayout *set_layouts = NULL;
    mt_array_pushn(dev->alloc, set_layouts, SETS_PER_PAGE);
    for (uint32_t i = 0; i < mt_array_size(set_layouts); i++) {
        set_layouts[i] = p->set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = *pool,
        .descriptorSetCount = SETS_PER_PAGE,
        .pSetLayouts        = set_layouts,
    };

    VkDescriptorSet **sets = mt_array_push(dev->alloc, p->set_arrays, NULL);
    mt_array_pushn(dev->alloc, *sets, SETS_PER_PAGE);
    VK_CHECK(vkAllocateDescriptorSets(dev->device, &alloc_info, *sets));

    mt_array_free(dev->alloc, set_layouts);
}

static void descriptor_pool_init(
    MtDevice *dev,
    DescriptorPool *p,
    PipelineLayout *layout,
    uint32_t set_index) {
    memset(p, 0, sizeof(*p));

    // Create set layout
    {
        VkDescriptorSetLayoutCreateInfo set_layout_create_info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = mt_array_size(layout->sets[set_index].bindings),
            .pBindings    = layout->sets[set_index].bindings,
        };

        VK_CHECK(vkCreateDescriptorSetLayout(
            dev->device, &set_layout_create_info, NULL, &p->set_layout));
    }

    // Create update template
    {
        SetInfo *set                             = &layout->sets[set_index];
        VkDescriptorUpdateTemplateEntry *entries = NULL;
        for (uint32_t b = 0; b < mt_array_size(set->bindings); b++) {
            VkDescriptorSetLayoutBinding *binding = &set->bindings[b];

            VkDescriptorUpdateTemplateEntry entry = {
                .dstBinding      = binding->binding,
                .dstArrayElement = 0,
                .descriptorCount = binding->descriptorCount,
                .descriptorType  = binding->descriptorType,
                .offset          = binding->binding * sizeof(Descriptor),
                .stride          = sizeof(Descriptor),
            };
            mt_array_push(dev->alloc, entries, entry);
        }

        VkDescriptorUpdateTemplateCreateInfo template_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
            .descriptorUpdateEntryCount = mt_array_size(entries),
            .pDescriptorUpdateEntries   = entries,
            .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
            .descriptorSetLayout = p->set_layout,
            .pipelineBindPoint   = layout->bind_point,
            .pipelineLayout      = layout->layout,
            .set                 = set_index,
        };

        vkCreateDescriptorUpdateTemplate(
            dev->device, &template_info, NULL, &p->update_template);

        mt_array_free(dev->alloc, entries);
    }

    // Setup up pool sizes
    {
        VkDescriptorSetLayoutBinding *binding;
        mt_array_foreach(binding, layout->sets[set_index].bindings) {
            VkDescriptorPoolSize *found_pool_size = NULL;

            VkDescriptorPoolSize *pool_size = NULL;
            mt_array_foreach(pool_size, p->pool_sizes) {
                if (pool_size->type == binding->descriptorType) {
                    found_pool_size = pool_size;
                    break;
                }
            }

            if (!found_pool_size) {
                found_pool_size = mt_array_push(
                    dev->alloc, p->pool_sizes, (VkDescriptorPoolSize){0});

                found_pool_size->type            = binding->descriptorType;
                found_pool_size->descriptorCount = 0;
            }

            found_pool_size->descriptorCount += SETS_PER_PAGE;
        }

        assert(mt_array_size(p->pool_sizes) > 0);
    }

    // Setup initial descriptor pool
    descriptor_pool_grow(dev, p);
}

static VkDescriptorSet descriptor_pool_alloc(
    MtDevice *dev,
    DescriptorPool *p,
    Descriptor *descriptors,
    uint64_t descriptors_hash) {
    for (uint32_t i = 0; i < mt_array_size(p->pools); i++) {
        VkDescriptorPool pool         = p->pools[i];
        uint32_t *allocated_set_count = &p->allocated_set_counts[i];
        MtHashMap *hashmap            = &p->pool_hashmaps[i];
        VkDescriptorSet *sets         = p->set_arrays[i];

        VkDescriptorSet *set = mt_hash_get_ptr(hashmap, descriptors_hash);

        if (set != NULL) {
            // Set is available
            return *set;
        } else {
            if (*allocated_set_count >= SETS_PER_PAGE) {
                // No sets available in this pool, so create a new pool
                descriptor_pool_grow(dev, p);
                return descriptor_pool_alloc(
                    dev, p, descriptors, descriptors_hash);
            }

            // Update existing descriptor set, because we haven't found any
            // matching ones already

            (*allocated_set_count)++;
            set = &sets[(*allocated_set_count) - 1];

            vkUpdateDescriptorSetWithTemplate(
                dev->device, *set, p->update_template, descriptors);

            mt_hash_set_ptr(hashmap, descriptors_hash, set);

            return *set;
        }
    }

    assert(0);
}

static void descriptor_pool_reset(MtDevice *dev, DescriptorPool *p) {
    for (uint32_t i = 0; i < mt_array_size(p->allocated_set_counts); i++) {
        p->allocated_set_counts[i] = 0;
    }
}

static void descriptor_pool_destroy(MtDevice *dev, DescriptorPool *p) {
    for (uint32_t i = 0; i < mt_array_size(p->pools); i++) {
        vkDestroyDescriptorPool(dev->device, p->pools[i], NULL);
    }
    mt_array_free(dev->alloc, p->pools);

    mt_array_free(dev->alloc, p->allocated_set_counts);

    for (uint32_t i = 0; i < mt_array_size(p->pool_hashmaps); i++) {
        mt_hash_destroy(&p->pool_hashmaps[i]);
    }
    mt_array_free(dev->alloc, p->pool_hashmaps);

    for (uint32_t i = 0; i < mt_array_size(p->set_arrays); i++) {
        mt_array_free(dev->alloc, p->set_arrays[i]);
    }
    mt_array_free(dev->alloc, p->set_arrays);

    mt_array_free(dev->alloc, p->pool_sizes);

    vkDestroyDescriptorSetLayout(dev->device, p->set_layout, NULL);
    vkDestroyDescriptorUpdateTemplate(dev->device, p->update_template, NULL);
}
