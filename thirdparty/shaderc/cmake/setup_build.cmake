if(NOT COMMAND find_host_package)
  macro(find_host_package)
    find_package(${ARGN})
  endmacro()
endif()
if(NOT COMMAND find_host_program)
  macro(find_host_program)
    find_program(${ARGN})
  endmacro()
endif()

# Find asciidoctor; see shaderc_add_asciidoc() from utils.cmake for
# adding documents.
find_program(ASCIIDOCTOR_EXE NAMES asciidoctor)
if (NOT ASCIIDOCTOR_EXE)
  message(STATUS "asciidoctor was not found - no documentation will be"
          " generated")
endif()

# On Windows, CMake by default compiles with the shared CRT.
# Ensure that gmock compiles the same, otherwise failures will occur.
if(WIN32)
  # TODO(awoloszyn): Once we support selecting CRT versions,
  # make sure this matches correctly.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif(WIN32)


option(ENABLE_CODE_COVERAGE "Enable collecting code coverage." OFF)

option(DISABLE_RTTI "Disable RTTI in builds")
if(DISABLE_RTTI)
  if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fno-rtti")
  endif(UNIX)
endif(DISABLE_RTTI)

option(DISABLE_EXCEPTIONS "Disables exceptions in builds")
if(DISABLE_EXCEPTIONS)
  if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  endif(UNIX)
endif(DISABLE_EXCEPTIONS)
