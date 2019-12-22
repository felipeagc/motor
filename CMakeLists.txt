cmake_minimum_required(VERSION 3.12)
project(Motor)

set(CMAKE_BUILD_TYPE Debug)
enable_testing()
find_package (Threads)

set(
	MOTOR_SOURCES
	src/arena.c
	src/xxhash.c
	src/vulkan/device.c
	src/vulkan/volk.c
	src/vulkan/spirv_reflect.c
	src/vulkan/vk_mem_alloc.cpp
	)

add_library(motor ${MOTOR_SOURCES})
target_include_directories(motor PUBLIC src/)
target_link_libraries(motor ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

add_executable(simple examples/simple.c)
target_link_libraries(simple PUBLIC motor)

add_executable(arena_tests tests/arena_tests.c)
target_link_libraries(arena_tests PUBLIC motor)
add_test(area_tests arena_tests)
