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
	MOTOR_SOURCES
	src/motor/allocator.c
	include/motor/allocator.h

	src/motor/arena.c
	include/motor/arena.h

	src/motor/bump_alloc.c
	include/motor/bump_alloc.h

	src/motor/renderer.c
	include/motor/renderer.h

	src/motor/threads.c
	include/motor/threads.h

	src/motor/array.c
	include/motor/array.h

	src/motor/hashmap.c
	include/motor/hashmap.h

	src/motor/bitset.c
	include/motor/bitset.h

	src/motor/xxhash.c
	src/motor/xxhash.h
	src/motor/xxh3.h

	src/motor/file_watcher.c
	include/motor/file_watcher.h

	src/motor/config.c
	include/motor/config.h

	src/motor/string_builder.c
	include/motor/string_builder.h

	src/motor/engine.c
	include/motor/engine.h

	src/motor/asset_manager.c
	include/motor/asset_manager.h

	src/motor/stb_image.c
	src/motor/stb_image.h

	src/motor/assets/image_asset.c
	include/motor/assets/image_asset.h

	src/motor/assets/pipeline_asset.c
	include/motor/assets/pipeline_asset.h

	src/motor/vulkan/glfw_window.c
	include/motor/vulkan/glfw_window.h

	src/motor/vulkan/vulkan_device.c
	include/motor/vulkan/vulkan_device.h

	src/motor/vulkan/volk.c
	src/motor/vulkan/volk.h
	src/motor/vulkan/spirv_reflect.c
	src/motor/vulkan/spirv_reflect.h
	src/motor/vulkan/vk_mem_alloc.cpp
	src/motor/vulkan/vk_mem_alloc.h
	)

add_library(motor ${MOTOR_SOURCES})
target_include_directories(motor PUBLIC include/ PRIVATE ${glfw_INCLUDE_DIRS})
target_link_libraries(motor ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} vulkan-headers glfw shaderc_shared)

add_executable(simple examples/simple.c)
target_link_libraries(simple PUBLIC motor)

add_executable(watcher examples/watcher.c)
target_link_libraries(watcher PUBLIC motor)

add_executable(read_config examples/read_config.c)
target_link_libraries(read_config PUBLIC motor)

add_executable(arena_tests tests/arena_tests.c)
target_link_libraries(arena_tests PUBLIC motor)
add_test(area_tests arena_tests)

add_executable(hash_tests tests/hash_tests.c)
target_link_libraries(hash_tests PUBLIC motor)
add_test(hash_tests hash_tests)

