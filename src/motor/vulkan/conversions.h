#ifndef MT_VULKAN_CONVERSIONS
#define MT_VULKAN_CONVERSIONS

#include "internal.h"
#include "../../../include/motor/renderer.h"

static MtFormat format_from_vulkan(VkFormat format) {
  switch (format) {
  case VK_FORMAT_R8_UINT: return MT_FORMAT_R8_UINT;
  case VK_FORMAT_R32_UINT: return MT_FORMAT_R32_UINT;

  case VK_FORMAT_R8G8B8_UNORM: return MT_FORMAT_RGB8_UNORM;
  case VK_FORMAT_R8G8B8A8_UNORM: return MT_FORMAT_RGBA8_UNORM;

  case VK_FORMAT_B8G8R8A8_UNORM: return MT_FORMAT_BGRA8_UNORM;

  case VK_FORMAT_R32_SFLOAT: return MT_FORMAT_R32_SFLOAT;
  case VK_FORMAT_R32G32_SFLOAT: return MT_FORMAT_RG32_SFLOAT;
  case VK_FORMAT_R32G32B32_SFLOAT: return MT_FORMAT_RGB32_SFLOAT;
  case VK_FORMAT_R32G32B32A32_SFLOAT: return MT_FORMAT_RGBA32_SFLOAT;

  case VK_FORMAT_R16G16B16A16_SFLOAT: return MT_FORMAT_RGBA16_SFLOAT;

  case VK_FORMAT_D16_UNORM: return MT_FORMAT_D16_UNORM;
  case VK_FORMAT_D16_UNORM_S8_UINT: return MT_FORMAT_D16_UNORM_S8_UINT;
  case VK_FORMAT_D24_UNORM_S8_UINT: return MT_FORMAT_D24_UNORM_S8_UINT;
  case VK_FORMAT_D32_SFLOAT: return MT_FORMAT_D32_SFLOAT;
  case VK_FORMAT_D32_SFLOAT_S8_UINT: return MT_FORMAT_D32_SFLOAT_S8_UINT;

  default: return MT_FORMAT_UNDEFINED;
  }
}

static VkFormat format_to_vulkan(MtFormat format) {
  switch (format) {
  case MT_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
  case MT_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;

  case MT_FORMAT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
  case MT_FORMAT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;

  case MT_FORMAT_BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;

  case MT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
  case MT_FORMAT_RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
  case MT_FORMAT_RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
  case MT_FORMAT_RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

  case MT_FORMAT_RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;

  case MT_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
  case MT_FORMAT_D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT;
  case MT_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
  case MT_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
  case MT_FORMAT_D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

  default: return VK_FORMAT_UNDEFINED;
  }
}

static MtIndexType index_type_from_vulkan(VkIndexType index_type) {
  switch (index_type) {
  case VK_INDEX_TYPE_UINT16: return MT_INDEX_TYPE_UINT16;
  case VK_INDEX_TYPE_UINT32: return MT_INDEX_TYPE_UINT32;
  }
}

static VkIndexType index_type_to_vulkan(MtIndexType index_type) {
  switch (index_type) {
  case MT_INDEX_TYPE_UINT16: return VK_INDEX_TYPE_UINT16;
  case MT_INDEX_TYPE_UINT32: return VK_INDEX_TYPE_UINT32;
  }
}

static MtCullMode cull_mode_from_vulkan(VkCullModeFlagBits cull_mode) {
  switch (cull_mode) {
  case VK_CULL_MODE_NONE: return MT_CULL_MODE_NONE;
  case VK_CULL_MODE_BACK_BIT: return MT_CULL_MODE_BACK;
  case VK_CULL_MODE_FRONT_BIT: return MT_CULL_MODE_FRONT;
  case VK_CULL_MODE_FRONT_AND_BACK: return MT_CULL_MODE_FRONT_AND_BACK;
  }
}

static VkCullModeFlagBits cull_mode_to_vulkan(MtCullMode cull_mode) {
  switch (cull_mode) {
  case MT_CULL_MODE_NONE: return VK_CULL_MODE_NONE;
  case MT_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
  case MT_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
  case MT_CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
  }
}

static MtFrontFace front_face_from_vulkan(VkFrontFace front_face) {
  switch (front_face) {
  case VK_FRONT_FACE_CLOCKWISE: return MT_FRONT_FACE_CLOCKWISE;
  case VK_FRONT_FACE_COUNTER_CLOCKWISE: return MT_FRONT_FACE_COUNTER_CLOCKWISE;
  }
}

static VkFrontFace front_face_to_vulkan(MtFrontFace front_face) {
  switch (front_face) {
  case MT_FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
  case MT_FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }
}

#endif
