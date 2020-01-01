cmake_minimum_required(VERSION 2.8.12)
project(shaderc)
enable_testing()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# message(STATUS "Shaderc: build type is \"${CMAKE_BUILD_TYPE}\".")

option(SHADERC_ENABLE_SPVC "Enable libshaderc_spvc" OFF)

set (CMAKE_CXX_STANDARD 11)

option(SHADERC_ENABLE_NV_EXTENSIONS "Enables support of Nvidia-specific extensions" ON)

include(GNUInstallDirs)
include(cmake/setup_build.cmake)
include(cmake/utils.cmake)
include(CheckCXXCompilerFlag)

set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ${DISABLE_EXCEPTIONS} CACHE BOOL "Coupling SPIRV-Cross exception conversion to DISABLE_EXCEPTIONS" FORCE)
if(DISABLE_EXCEPTIONS)
  # Need to set additional values here, since some of the wrapped code occurs in
  # .h/.hpp files, so maybe included outside of the library.
 add_definitions(-DSPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
endif()

# These flags are not supported on Windows and some older version of GCC
# that our bots use.
# Warning about implicit fallthrough in switch blocks
check_cxx_compiler_flag(-Wimplicit-fallthrough COMPILER_SUPPORTS_FALLTHROUGH_WARNING)
if (COMPILER_SUPPORTS_FALLTHROUGH_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wimplicit-fallthrough")
endif()

# Warning about extra semi-colons
check_cxx_compiler_flag(-Wextra-semi COMPILER_SUPPORTS_EXTRA_SEMI_WARNING)
if (COMPILER_SUPPORTS_EXTRA_SEMI_WARNING)
    add_compile_options("-Wextra-semi")
endif()

if(SHADERC_ENABLE_NV_EXTENSIONS)
    add_definitions(-DNV_EXTENSIONS)
endif(SHADERC_ENABLE_NV_EXTENSIONS)

if(MSVC)
  option(SHADERC_ENABLE_SHARED_CRT
          "Use the shared CRT instead of the static CRT"
          ${SHADERC_ENABLE_SHARED_CRT})
  if (NOT SHADERC_ENABLE_SHARED_CRT)
    # Link executables statically by replacing /MD with /MT everywhere.
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  endif(NOT SHADERC_ENABLE_SHARED_CRT)
endif(MSVC)


# Configure subdirectories.
# We depend on these for later projects, so they should come first.
add_subdirectory(third_party)

if(SHADERC_ENABLE_SPVC)
add_subdirectory(libshaderc_spvc)
add_subdirectory(spvc)
endif()
add_subdirectory(libshaderc_util)
add_subdirectory(libshaderc)
