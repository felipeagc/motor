cmake_minimum_required(VERSION 3.12)
project(Motor)

set(CMAKE_BUILD_TYPE Debug)
enable_testing()
find_package (Threads)

add_subdirectory(thirdparty)

set(
	MOTOR_SOURCES
	src/motor/arena.c
	src/motor/renderer.c
	src/motor/threads.c
	src/motor/array.c
	src/motor/hashmap.c
	src/motor/bitset.c
	src/motor/xxhash.c

	src/motor/vulkan/glfw_window.c
	src/motor/vulkan/vulkan_device.c
	src/motor/vulkan/volk.c
	src/motor/vulkan/spirv_reflect.c
	src/motor/vulkan/vk_mem_alloc.cpp
	)

add_library(motor ${MOTOR_SOURCES})
target_include_directories(motor PUBLIC include/ PRIVATE ${glfw_INCLUDE_DIRS})
target_link_libraries(motor ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} glfw)

add_executable(simple examples/simple.c)
target_link_libraries(simple PUBLIC motor)

add_executable(arena_tests tests/arena_tests.c)
target_link_libraries(arena_tests PUBLIC motor)
add_test(area_tests arena_tests)

add_executable(hash_tests tests/hash_tests.c)
target_link_libraries(hash_tests PUBLIC motor)
add_test(hash_tests hash_tests)

