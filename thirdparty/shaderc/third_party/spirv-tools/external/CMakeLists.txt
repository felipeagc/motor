# Copyright (c) 2015-2016 The Khronos Group Inc.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if (DEFINED SPIRV-Headers_SOURCE_DIR)
  # This allows flexible position of the SPIRV-Headers repo.
  set(SPIRV_HEADER_DIR ${SPIRV-Headers_SOURCE_DIR})
else()
  if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Headers)
    set(SPIRV_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Headers)
  else()
    set(SPIRV_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spirv-headers)
  endif()
endif()

if (IS_DIRECTORY ${SPIRV_HEADER_DIR})
  set(SPIRV_HEADER_INCLUDE_DIR ${SPIRV_HEADER_DIR}/include PARENT_SCOPE)
else()
  message(FATAL_ERROR
    "SPIRV-Headers was not found - please checkout a copy under external/.")
endif()

if(SPIRV_BUILD_FUZZER)
  set(PROTOBUF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/cmake)
  set(protobuf_BUILD_TESTS OFF CACHE BOOL "Disable protobuf tests")
  set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "Do not build protobuf static runtime")
  if (IS_DIRECTORY ${PROTOBUF_DIR})
    add_subdirectory(${PROTOBUF_DIR} EXCLUDE_FROM_ALL)
  else()
    message(FATAL_ERROR
      "protobuf not found - please checkout a copy under external/.")
  endif()
endif(SPIRV_BUILD_FUZZER)
