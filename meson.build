project('motor', 'c', 'cpp', 
  default_options: ['default_library=static'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)
dl_dep = cc.find_library('dl', required: false)
threads_dep = dependency('threads')

add_project_arguments(cc.get_supported_arguments(['-Wno-missing-braces']), language: 'c')

subdir('thirdparty')

motor_base_sources = [
  'src/motor/base/allocator.c',
  'src/motor/base/arena.c',
  'src/motor/base/bump_alloc.c',
  'src/motor/base/string_builder.c',
  'src/motor/base/threads.c',
  'src/motor/base/thread_pool.c',
  'src/motor/base/array.c',
  'src/motor/base/hashmap.c',
  'src/motor/base/bitset.c',
  'src/motor/base/lexer.c',
  'src/motor/base/time.c',
  'src/motor/base/log.c',

  'src/motor/base/xxhash.c',
]

motor_base_deps = [m_dep, dl_dep, threads_dep]

motor_base_lib = library(
  'motor_base',
  motor_base_sources,
  dependencies: motor_base_deps,
  include_directories: ['include']
  )

motor_base_dep = declare_dependency(
  link_with: motor_base_lib,
  dependencies: motor_base_deps,
  include_directories: ['include']
  )

motor_graphics_sources = [
  'src/motor/graphics/renderer.c',
  'src/motor/graphics/window.c',
  'src/motor/graphics/vulkan/glfw_window.c',
  'src/motor/graphics/vulkan/vulkan_device.c',

  'src/motor/graphics/vulkan/volk.c',
  'src/motor/graphics/vulkan/spirv_reflect.c',
  'src/motor/graphics/vulkan/vk_mem_alloc.cpp',
]

motor_graphics_deps = [motor_base_dep, glfw_dep]

motor_graphics_lib = library(
  'motor_graphics',
  motor_graphics_sources,
  dependencies: motor_graphics_deps,
  include_directories: ['include', 'thirdparty/vulkan-headers']
  )

motor_graphics_dep = declare_dependency(
  link_with: motor_graphics_lib,
  dependencies: motor_graphics_deps,
  include_directories: ['include'])

motor_engine_sources = [
  'src/motor/engine/file_watcher.c',
  'src/motor/engine/config.c',
  'src/motor/engine/engine.c',
  'src/motor/engine/ui.c',
  'src/motor/engine/camera.c',
  'src/motor/engine/environment.c',
  'src/motor/engine/entities.c',
  'src/motor/engine/asset_manager.c',
  'src/motor/engine/assets/image_asset.c',
  'src/motor/engine/assets/pipeline_asset.c',
  'src/motor/engine/assets/font_asset.c',
  'src/motor/engine/assets/gltf_asset.c',
  'src/motor/engine/cgltf.c',
  'src/motor/engine/tinyktx.c',
  'src/motor/engine/stb_image.c',
  'src/motor/engine/stb_rect_pack.c',
  'src/motor/engine/stb_truetype.c',
]

if target_machine.system() == 'windows'
  shaderc_dep = cc.find_library('shaderc_shared', 
	dirs: [join_paths(meson.source_root(), 'thirdparty/shaderc/windows')],
	static: false)
  configure_file(
    input: 'thirdparty/shaderc/windows/shaderc_shared.dll',
    output: 'shaderc_shared.dll',
    copy: true)
else
  shaderc_dep = cc.find_library('shaderc', 
	dirs: [join_paths(meson.source_root(), 'thirdparty/shaderc/linux')],
	static: true)
endif
motor_engine_deps = [motor_base_dep, motor_graphics_dep, shaderc_dep]

motor_engine_lib = library(
  'motor_engine',
  motor_engine_sources,
  dependencies: motor_engine_deps,
  include_directories: ['include', 'thirdparty/shaderc']
  )

motor_engine_dep = declare_dependency(
  link_with: motor_engine_lib,
  dependencies: motor_engine_deps,
  include_directories: ['include']
  )

executable('gltf_ktx', ['tools/gltf_ktx.c', 'tools/bc7enc16.c'], dependencies: [motor_base_dep])

executable('simple', 'examples/simple.c', dependencies: [motor_engine_dep])
executable('watcher', 'examples/watcher.c', dependencies: [motor_engine_dep])
executable('read_config', 'examples/read_config.c', dependencies: [motor_engine_dep])
executable('tokenizer', 'examples/tokenizer.c', dependencies: [motor_base_dep])

arena_tests = executable('arena_tests', 'tests/arena_tests.c', dependencies: [motor_base_dep])
test('arena', arena_tests)

hash_tests = executable('hash_tests', 'tests/hash_tests.c', dependencies: [motor_base_dep])
test('hash', hash_tests)

thread_tests = executable('thread_tests', 'tests/thread_tests.c', dependencies: [motor_base_dep])
test('thread', thread_tests)
