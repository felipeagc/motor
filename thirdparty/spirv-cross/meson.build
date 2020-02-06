spirv_cross_sources = [
  'spirv_cfg.cpp',
  'spirv_cpp.cpp',
  'spirv_cpp.hpp',
  'spirv_cross_c.cpp',
  'spirv_cross.cpp',
  'spirv_cross_parsed_ir.cpp',
  'spirv_cross_util.cpp',
  'spirv_glsl.cpp',
  'spirv_hlsl.cpp',
  'spirv_msl.cpp',
  'spirv_parser.cpp',
  'spirv_reflect.cpp',
]

spirv_cross_include_dirs = ['.']

spirv_cross_lib = library(
  'spirv_cross',
  spirv_cross_sources,
  include_directories: spirv_cross_include_dirs)
spirv_cross_dep = declare_dependency(
  link_with: spirv_cross_lib,
  include_directories: spirv_cross_include_dirs)
