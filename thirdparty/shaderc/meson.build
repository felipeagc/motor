shaderc_include_dir = include_directories('shaderc')

if target_machine.system() == 'windows'
  shaderc_lib_dir = meson.current_source_dir() / 'windows'
  shaderc_dep = cc.find_library('shaderc_shared', dirs: [shaderc_lib_dir])
  configure_file(
    input: 'windows/shaderc_shared.dll',
    output: 'shaderc_shared.dll',
    copy: true)
  configure_file(
    input: 'windows/shaderc_shared.lib',
    output: 'shaderc_shared.lib',
    copy: true)
else
  shaderc_lib_dir = meson.current_source_dir() / 'linux'
  shaderc_dep = cc.find_library('shaderc', dirs: [shaderc_lib_dir])
endif
