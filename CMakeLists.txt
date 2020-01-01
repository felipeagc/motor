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
	src/motor/arena.c
	src/motor/bump_alloc.c
	src/motor/renderer.c
	src/motor/threads.c
	src/motor/array.c
	src/motor/hashmap.c
	src/motor/bitset.c
	src/motor/xxhash.c
	src/motor/file_watcher.c
	src/motor/config.c
	src/motor/string_builder.c
	src/motor/engine.c
	src/motor/asset_manager.c
	src/motor/stb_image.c

	src/motor/assets/image_asset.c
	src/motor/assets/pipeline_asset.c

	src/motor/vulkan/glfw_window.c
	src/motor/vulkan/vulkan_device.c
	src/motor/vulkan/volk.c
	src/motor/vulkan/spirv_reflect.c
	src/motor/vulkan/vk_mem_alloc.cpp
	)

add_library(motor ${MOTOR_SOURCES})
target_include_directories(motor PUBLIC include/ PRIVATE ${glfw_INCLUDE_DIRS})
target_link_libraries(motor ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} glfw shaderc_shared)

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

