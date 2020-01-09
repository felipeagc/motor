cmake_minimum_required(VERSION 3.12)
project(Motor)

set(CMAKE_BUILD_TYPE Debug)
enable_testing()
find_package (Threads)

add_subdirectory(thirdparty)

if (WIN32)
else()
	set(CMAKE_CC_FLAGS "${CMAKE_CC_FLAGS} -Wall -pedantic -Wpedantic -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wpedantic -Wextra")
endif()

set(
	MOTOR_BASE_SOURCES
	src/motor/base/allocator.c
	include/motor/base/allocator.h

	src/motor/base/arena.c
	include/motor/base/arena.h

	src/motor/base/bump_alloc.c
	include/motor/base/bump_alloc.h

	src/motor/base/string_builder.c
	include/motor/base/string_builder.h

	src/motor/base/threads.c
	include/motor/base/threads.h

	src/motor/base/array.c
	include/motor/base/array.h

	src/motor/base/hashmap.c
	include/motor/base/hashmap.h

	src/motor/base/bitset.c
	include/motor/base/bitset.h

	src/motor/base/xxhash.c
	src/motor/base/xxhash.h
	src/motor/base/xxh3.h
	)

set(
	MOTOR_GRAPHICS_SOURCES

	src/motor/graphics/renderer.c
	include/motor/graphics/renderer.h

	src/motor/graphics/window.c
	include/motor/graphics/window.h

	src/motor/graphics/vulkan/glfw_window.c
	include/motor/graphics/vulkan/glfw_window.h

	src/motor/graphics/vulkan/vulkan_device.c
	include/motor/graphics/vulkan/vulkan_device.h

	# Third party stuff:
	src/motor/graphics/vulkan/volk.c
	src/motor/graphics/vulkan/volk.h
	src/motor/graphics/vulkan/spirv_reflect.c
	src/motor/graphics/vulkan/spirv_reflect.h
	src/motor/graphics/vulkan/vk_mem_alloc.cpp
	src/motor/graphics/vulkan/vk_mem_alloc.h
	)

set(
	MOTOR_ENGINE_SOURCES

	src/motor/engine/file_watcher.c
	include/motor/engine/file_watcher.h

	src/motor/engine/config.c
	include/motor/engine/config.h

	src/motor/engine/engine.c
	include/motor/engine/engine.h

	src/motor/engine/ui.c
	include/motor/engine/ui.h

	src/motor/engine/camera.c
	include/motor/engine/camera.h

	src/motor/engine/environment.c
	include/motor/engine/environment.h

	src/motor/engine/asset_manager.c
	include/motor/engine/asset_manager.h

	src/motor/engine/assets/image_asset.c
	include/motor/engine/assets/image_asset.h

	src/motor/engine/assets/pipeline_asset.c
	include/motor/engine/assets/pipeline_asset.h

	src/motor/engine/assets/font_asset.c
	src/motor/engine/assets/font_asset.inl
	include/motor/engine/assets/font_asset.h

	src/motor/engine/assets/gltf_asset.c
	include/motor/engine/assets/gltf_asset.h

	src/motor/engine/tinyktx.c
	src/motor/engine/tinyktx.h

	# Third party stuff:
	src/motor/engine/stb_image.c
	src/motor/engine/stb_image.h
	src/motor/engine/stb_rect_pack.c
	src/motor/engine/stb_rect_pack.h
	src/motor/engine/stb_truetype.c
	src/motor/engine/stb_truetype.h
	)

add_library(motor_base ${MOTOR_BASE_SOURCES})
target_include_directories(motor_base PUBLIC include/)
target_link_libraries(motor_base ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

add_library(motor_graphics ${MOTOR_GRAPHICS_SOURCES})
target_include_directories(motor_graphics PUBLIC include/ thirdparty/shaderc/)
target_link_libraries(motor_graphics motor_base vulkan-headers glfw)

if(WIN32)
	target_link_libraries(motor_graphics ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shaderc/shaderc_shared.lib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_link_libraries(motor_graphics ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/shaderc/libshaderc_shared.so.1)
endif()

add_library(motor_engine ${MOTOR_ENGINE_SOURCES})
target_include_directories(motor_engine PUBLIC include/)
target_link_libraries(motor_engine PUBLIC motor_base motor_graphics)

add_executable(simple examples/simple.c)
target_link_libraries(simple PUBLIC motor_engine)

add_executable(watcher examples/watcher.c)
target_link_libraries(watcher PUBLIC motor_engine)

add_executable(read_config examples/read_config.c)
target_link_libraries(read_config PUBLIC motor_engine)

add_executable(arena_tests tests/arena_tests.c)
target_link_libraries(arena_tests PUBLIC motor_base)
add_test(area_tests arena_tests)

add_executable(hash_tests tests/hash_tests.c)
target_link_libraries(hash_tests PUBLIC motor_base)
add_test(hash_tests hash_tests)

