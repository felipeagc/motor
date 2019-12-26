#include "internal.h"

#include "../xxhash.h"

enum { MAX_SET_AGE = 8 }; // age in frames

// Page {{{
static void ds_page_begin_frame(DSAllocatorPage *page) {
  page->last_index = 0;
  for (uint32_t i = 0; i < mt_array_size(page->set_ages); i++) {
    if (mt_bitset_get(&page->in_use, i)) {
      page->set_ages[i]++;
      if (page->set_ages[i] > MAX_SET_AGE) {
        mt_bitset_disable(&page->in_use, i);
        mt_hash_remove(&page->hashmap, page->hashes[i]);
      }
    }
  }
}

static void ds_page_init(
    DSAllocatorPage *page, DSAllocator *allocator, uint32_t set_index) {
  memset(page, 0, sizeof(*page));
  page->set_index = set_index;

  // Create descriptor pool
  VkDescriptorPoolCreateInfo create_info = {
      .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets       = SETS_PER_PAGE,
      .poolSizeCount = mt_array_size(allocator->pool_sizes[set_index]),
      .pPoolSizes    = allocator->pool_sizes[set_index],
  };

  VK_CHECK(vkCreateDescriptorPool(
      allocator->dev->device, &create_info, NULL, &page->pool));

  VkDescriptorSetLayout *set_layouts = NULL;
  mt_array_pushn(allocator->dev->arena, set_layouts, SETS_PER_PAGE);

  VkDescriptorSetLayout *set_layout;
  mt_array_foreach(set_layout, set_layouts) {
    *set_layout = allocator->set_layouts[set_index];
  }

  VkDescriptorSetAllocateInfo alloc_info = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = page->pool,
      .descriptorSetCount = SETS_PER_PAGE,
      .pSetLayouts        = set_layouts,
  };

  mt_array_pushn(allocator->dev->arena, page->sets, SETS_PER_PAGE);
  mt_array_pushn(allocator->dev->arena, page->set_ages, SETS_PER_PAGE);
  memset(
      page->set_ages,
      0,
      sizeof(*page->set_ages) * mt_array_size(page->set_ages));
  mt_array_pushn(allocator->dev->arena, page->hashes, SETS_PER_PAGE);
  mt_hash_init(&page->hashmap, SETS_PER_PAGE, allocator->dev->arena);

  VK_CHECK(vkAllocateDescriptorSets(
      allocator->dev->device, &alloc_info, page->sets));

  ds_page_begin_frame(page);

  mt_array_free(allocator->dev->arena, set_layouts);
}

static void ds_page_destroy(DSAllocatorPage *page, DSAllocator *allocator) {
  if (page->next) ds_page_destroy(page->next, allocator);

  mt_free(allocator->dev->arena, page->next);
  mt_array_free(allocator->dev->arena, page->sets);
  mt_array_free(allocator->dev->arena, page->set_ages);
  mt_array_free(allocator->dev->arena, page->hashes);
  mt_hash_destroy(&page->hashmap);

  VK_CHECK(vkDeviceWaitIdle(allocator->dev->device));
  vkDestroyDescriptorPool(allocator->dev->device, page->pool, NULL);
}

static VkDescriptorSet ds_page_find(
    DSAllocatorPage *page, DSAllocator *allocator, Descriptor *descriptors) {
  XXH64_state_t state = {0};
  XXH64_update(
      &state, descriptors, mt_array_size(descriptors) * sizeof(*descriptors));
  uint64_t descriptors_hash = (uint64_t)XXH64_digest(&state);

  uintptr_t addr = mt_hash_get(&page->hashmap, descriptors_hash);
  if (addr != MT_HASH_UNUSED) {
    VkDescriptorSet *set = (VkDescriptorSet *)addr;
    return *set;
  }

  return VK_NULL_HANDLE;
}

static VkDescriptorSet ds_page_allocate(
    DSAllocatorPage *page, DSAllocator *allocator, Descriptor *descriptors) {
  for (uint32_t i = page->last_index; i < mt_array_size(page->sets); i++) {
    VkDescriptorSet set = page->sets[i];

    if (!mt_bitset_get(&page->in_use, i)) {
      mt_bitset_enable(&page->in_use, i);

      page->set_ages[i] = 0;

      XXH64_state_t state = {0};
      XXH64_update(
          &state,
          descriptors,
          mt_array_size(descriptors) * sizeof(*descriptors));
      uint64_t descriptors_hash = (uint64_t)XXH64_digest(&state);

      page->hashes[i] = descriptors_hash;
      mt_hash_set(&page->hashmap, page->hashes[i], (uintptr_t)set);

      vkUpdateDescriptorSetWithTemplate(
          allocator->dev->device,
          page->sets[i],
          allocator->update_templates[page->set_index],
          descriptors);

      page->last_index = i + 1;

      return set;
    }
  }

  return VK_NULL_HANDLE;
}
// }}}

// DescriptorSetAllocator {{{
static void ds_allocator_begin_frame(DSAllocator *allocator) {
  allocator->current_frame = (allocator->current_frame + 1) % FRAMES_IN_FLIGHT;

  for (uint32_t i = 0; i < mt_array_size(allocator->set_layouts); i++) {
    // Reset last pages
    DSAllocatorFrame *frame = &allocator->frames[allocator->current_frame];
    frame->last_pages[i]    = &frame->base_pages[i];

    DSAllocatorPage *page = &frame->base_pages[i];
    while (page) {
      ds_page_begin_frame(page);
      page = page->next;
    }
  }
}

static void ds_allocator_init(
    DSAllocator *allocator, MtDevice *dev, PipelineLayout *pipeline_layout) {
  memset(allocator, 0, sizeof(*allocator));

  allocator->dev = dev;

  mt_array_pushn(
      dev->arena,
      allocator->set_layouts,
      mt_array_size(pipeline_layout->set_layouts));
  memcpy(
      allocator->set_layouts,
      pipeline_layout->set_layouts,
      sizeof(*pipeline_layout->set_layouts) *
          mt_array_size(pipeline_layout->set_layouts));

  mt_array_pushn(
      dev->arena,
      allocator->update_templates,
      mt_array_size(pipeline_layout->update_templates));
  memcpy(
      allocator->update_templates,
      pipeline_layout->update_templates,
      sizeof(*pipeline_layout->update_templates) *
          mt_array_size(pipeline_layout->update_templates));

  mt_array_pushn(
      dev->arena, allocator->pool_sizes, mt_array_size(allocator->set_layouts));
  memset(
      allocator->pool_sizes,
      0,
      sizeof(*allocator->pool_sizes) * mt_array_size(allocator->pool_sizes));

  for (uint32_t i = 0; i < mt_array_size(allocator->pool_sizes); i++) {
    VkDescriptorPoolSize *set_pool_sizes = allocator->pool_sizes[i];

    SetInfo *set = &pipeline_layout->sets[i];

    VkDescriptorSetLayoutBinding *binding;
    mt_array_foreach(binding, set->bindings) {
      VkDescriptorPoolSize *found_pool_size;

      VkDescriptorPoolSize *pool_size;
      mt_array_foreach(pool_size, set_pool_sizes) {
        if (pool_size->type == binding->descriptorType) {
          found_pool_size = pool_size;
          break;
        }
      }

      if (!found_pool_size) {
        found_pool_size = mt_array_push(
            dev->arena, set_pool_sizes, (VkDescriptorPoolSize){0});
        found_pool_size->type            = binding->descriptorType;
        found_pool_size->descriptorCount = 0;
      }

      found_pool_size->descriptorCount += SETS_PER_PAGE;
    }
  }

  for (DSAllocatorFrame *frame = allocator->frames;
       frame != allocator->frames + MT_LENGTH(allocator->frames);
       frame++) {
    mt_array_pushn(
        dev->arena, frame->base_pages, mt_array_size(allocator->set_layouts));

    DSAllocatorPage *page;
    for (uint32_t i = 0; i < mt_array_size(frame->base_pages); i++) {
      ds_page_init(&frame->base_pages[i], allocator, i);
    }

    mt_array_pushn(
        dev->arena, frame->last_pages, mt_array_size(allocator->set_layouts));
  }

  ds_allocator_begin_frame(allocator);
}

static void ds_allocator_destroy(DSAllocator *allocator, MtDevice *dev) {
  for (DSAllocatorFrame *frame = allocator->frames;
       frame != allocator->frames + MT_LENGTH(allocator->frames);
       frame++) {

    for (uint32_t i = 0; i < mt_array_size(frame->base_pages); i++) {
      ds_page_destroy(&frame->base_pages[i], allocator);
    }

    mt_array_free(dev->arena, frame->base_pages);
    mt_array_free(dev->arena, frame->last_pages);
  }

  mt_array_free(dev->arena, allocator->set_layouts);
  mt_array_free(dev->arena, allocator->update_templates);
  for (uint32_t i = 0; i < mt_array_size(allocator->pool_sizes); i++) {
    mt_array_free(dev->arena, allocator->pool_sizes[i]);
  }
  mt_array_free(dev->arena, allocator->pool_sizes);
}

static VkDescriptorSet ds_allocator_allocate(
    DSAllocator *allocator, uint32_t set, Descriptor *descriptors) {
  DSAllocatorFrame *frame = &allocator->frames[allocator->current_frame];

  DSAllocatorPage *page = frame->last_pages[set];
  while (page) {
    VkDescriptorSet descriptor_set = ds_page_find(page, allocator, descriptors);
    if (descriptor_set) return descriptor_set;
    page = page->next;
  }

  page = frame->last_pages[set];
  while (page) {
    VkDescriptorSet descriptor_set =
        ds_page_allocate(page, allocator, descriptors);
    if (descriptor_set) return descriptor_set;
    if (!page->next) break;
    page = page->next;
  }

  // Could not allocate, so now we create a new page
  assert(!page->next);
  page->next = mt_alloc(allocator->dev->arena, sizeof(DSAllocatorPage));
  ds_page_init(page->next, allocator, set);
  page                   = page->next;
  frame->last_pages[set] = page;

  return ds_page_allocate(page, allocator, descriptors);
}
// }}}

static DSAllocator *
request_descriptor_set_allocator(MtDevice *dev, PipelineLayout *layout) {
  uintptr_t addr =
      mt_hash_get(&dev->descriptor_set_allocators, (uintptr_t)layout->layout);

  if (addr != MT_HASH_NOT_FOUND) {
    return (DSAllocator *)addr;
  }

  DSAllocator *alloc = mt_alloc(dev->arena, sizeof(DSAllocator));
  ds_allocator_init(alloc, dev, layout);
  mt_hash_set(
      &dev->descriptor_set_allocators,
      (uintptr_t)layout->layout,
      (uintptr_t)alloc);
  return alloc;
}
