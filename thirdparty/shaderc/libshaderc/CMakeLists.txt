project(libshaderc)

# Even though shaderc.hpp is a headers-only library, adding
# a dependency here will force clients of the library to rebuild
# when it changes.
set(SHADERC_SOURCES
  include/shaderc/shaderc.h
  include/shaderc/shaderc.hpp
  src/shaderc.cc
  src/shaderc_private.h
)

add_library(shaderc STATIC ${SHADERC_SOURCES})
shaderc_default_compile_options(shaderc)
target_include_directories(shaderc PUBLIC include PRIVATE ${glslang_SOURCE_DIR})

add_library(shaderc_shared SHARED ${SHADERC_SOURCES})
shaderc_default_compile_options(shaderc_shared)
target_include_directories(shaderc_shared PUBLIC include PRIVATE ${glslang_SOURCE_DIR})
target_compile_definitions(shaderc_shared
    PRIVATE SHADERC_IMPLEMENTATION
    PUBLIC SHADERC_SHAREDLIB
)
set_target_properties(shaderc_shared PROPERTIES SOVERSION 1)

find_package(Threads)
set(SHADERC_LIBS
  glslang OSDependent OGLCompiler glslang ${CMAKE_THREAD_LIBS_INIT}
  shaderc_util
  SPIRV # from glslang
  SPIRV-Tools
)

target_link_libraries(shaderc PRIVATE ${SHADERC_LIBS})
target_link_libraries(shaderc_shared PRIVATE ${SHADERC_LIBS})

shaderc_add_tests(
  TEST_PREFIX shaderc
  LINK_LIBS shaderc
  INCLUDE_DIRS include ${shaderc_SOURCE_DIR}/libshaderc_util/include ${glslang_SOURCE_DIR}
               ${spirv-tools_SOURCE_DIR}/include
  TEST_NAMES
    shaderc
    shaderc_cpp
    shaderc_private)

shaderc_add_tests(
  TEST_PREFIX shaderc_shared
  LINK_LIBS shaderc_shared SPIRV-Tools
  INCLUDE_DIRS include ${shaderc_SOURCE_DIR}/libshaderc_util/include ${glslang_SOURCE_DIR}
               ${spirv-tools_SOURCE_DIR}/include
  TEST_NAMES
    shaderc
    shaderc_cpp
    shaderc_private)

shaderc_combine_static_lib(shaderc_combined shaderc)

shaderc_add_tests(
  TEST_PREFIX shaderc_combined
  LINK_LIBS shaderc_combined ${CMAKE_THREAD_LIBS_INIT}
  INCLUDE_DIRS include ${shaderc_SOURCE_DIR}/libshaderc_util/include ${glslang_SOURCE_DIR}
               ${spirv-tools_SOURCE_DIR}/include
  TEST_NAMES
    shaderc
    shaderc_cpp)

if(${SHADERC_ENABLE_TESTS})
  add_executable(shaderc_c_smoke_test ./src/shaderc_c_smoke_test.c)
  shaderc_default_c_compile_options(shaderc_c_smoke_test)
  target_include_directories(shaderc_c_smoke_test PUBLIC ${shaderc_SOURCE_DIR}/libshaderc_util/include)
  target_link_libraries(shaderc_c_smoke_test PRIVATE shaderc)
  add_test(NAME shaderc_c_smoke_test COMMAND shaderc_c_smoke_test)
endif()
