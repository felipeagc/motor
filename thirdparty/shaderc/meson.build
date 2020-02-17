shaderc_include_dir = include_directories('.')

if target_machine.system() == 'windows'
  shaderc_lib_dir = meson.current_source_dir() / 'windows'
  shaderc_dep = cc.find_library('shaderc_shared', dirs: [shaderc_lib_dir])
else
  shaderc_lib_dir = meson.current_source_dir() / 'linux'
  shaderc_dep = cc.find_library('shaderc', dirs: [shaderc_lib_dir])
endif
