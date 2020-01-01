project(libshaderc)

find_package(Threads)

set(SPIRV_TOOLS "SPIRV-Tools")
set(SPVC_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  SPIRV-Tools
  SPIRV-Tools-opt
  spirv-cross-glsl
  spirv-cross-hlsl
  spirv-cross-msl
  spirv-cross-reflect
)

set(SPVC_SOURCES
  include/spvc/spvc.h
  include/spvc/spvc.hpp
  src/spvc.cc
  src/spvc_private.cc
  src/spvcir_pass.cc
  src/spvcir_pass.h
)

add_library(shaderc_spvc STATIC ${SPVC_SOURCES})
add_dependencies(shaderc_spvc ${SPIRV_TOOLS})
shaderc_default_compile_options(shaderc_spvc)
target_include_directories(shaderc_spvc
  PUBLIC include
  ${shaderc_SOURCE_DIR}/libshaderc/include
  ${spirv-tools_SOURCE_DIR}/include
  ${spirv-tools_SOURCE_DIR}
  ${spirv-tools_BINARY_DIR})
target_link_libraries(shaderc_spvc PRIVATE ${SPVC_LIBS})

add_library(shaderc_spvc_shared SHARED ${SPVC_SOURCES})
add_dependencies(shaderc_spvc_shared ${SPIRV_TOOLS}-shared)
shaderc_default_compile_options(shaderc_spvc_shared)
target_include_directories(shaderc_spvc_shared
  PUBLIC include
  ${shaderc_SOURCE_DIR}/libshaderc/include
  ${spirv-tools_SOURCE_DIR}/include
  ${spirv-tools_SOURCE_DIR}
  ${spirv-tools_BINARY_DIR})
target_link_libraries(shaderc_spvc_shared PRIVATE ${SPVC_LIBS})
set_target_properties(shaderc_spvc_shared PROPERTIES SOVERSION 1)
target_compile_definitions(shaderc_spvc_shared
    PRIVATE SHADERC_IMPLEMENTATION
    PUBLIC SHADERC_SHAREDLIB
)

target_compile_definitions(shaderc_spvc_shared
    PRIVATE SHADERC_IMPLEMENTATION
    PUBLIC SHADERC_SHAREDLIB
)


shaderc_add_tests(
  TEST_PREFIX shaderc
  LINK_LIBS shaderc_spvc ${SPVC_LIBS}
  INCLUDE_DIRS include ${shaderc_SOURCE_DIR}/libshaderc/include ${SPIRV-Cross_SOURCE_DIR}/..
    ${effcee_SOURCE_DIR}
    ${RE2_SOURCE_DIR}
  TEST_NAMES
    spvc
    spvc_cpp
    spvc_webgpu
    spvc_webgpu_cpp
    spvcir)


shaderc_combine_static_lib(shaderc_spvc_combined shaderc_spvc)

shaderc_add_tests(
  TEST_PREFIX shaderc_spvc_combined
  LINK_LIBS shaderc_spvc_combined ${SPVC_LIBS} ${CMAKE_THREAD_LIBS_INIT}
  INCLUDE_DIRS include ${shaderc_SOURCE_DIR}/libshaderc/include ${spirv-tools_SOURCE_DIR}/include
  TEST_NAMES
    spvc
    spvc_cpp)

if(${SHADERC_ENABLE_TESTS})
  add_executable(spvc_c_smoke_test ./src/spvc_smoke_test_util.c ./src/spvc_c_smoke_test.c)
  target_include_directories(spvc_c_smoke_test PUBLIC include PRIVATE ${shaderc_SOURCE_DIR}/libshaderc/include)
  shaderc_default_c_compile_options(spvc_c_smoke_test)
  target_link_libraries(spvc_c_smoke_test PRIVATE shaderc_spvc shaderc)
  add_test(NAME spvc_c_smoke_test COMMAND spvc_c_smoke_test)

  add_executable(spvc_webgpu_c_smoke_test ./src/spvc_smoke_test_util.c ./src/spvc_webgpu_c_smoke_test.c)
  target_include_directories(spvc_webgpu_c_smoke_test PUBLIC include PRIVATE ${shaderc_SOURCE_DIR}/libshaderc/include)
  shaderc_default_c_compile_options(spvc_webgpu_c_smoke_test)
  target_link_libraries(spvc_webgpu_c_smoke_test PRIVATE shaderc_spvc shaderc)
  add_test(NAME spvc_webgpu_c_smoke_test COMMAND spvc_webgpu_c_smoke_test)
endif()
