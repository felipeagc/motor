shaderc_include_dir = include_directories('shaderc')

if target_machine.system() == 'windows'
  shaderc_dep = cc.find_library('shaderc_shared', 
	dirs: [meson.source_root() / 'thirdparty/shaderc/windows'],
	static: false)
  configure_file(
    input: 'windows/shaderc_shared.dll',
    output: 'shaderc_shared.dll',
    copy: true)
  configure_file(
    input: 'windows/shaderc_shared.lib',
    output: 'shaderc_shared.lib',
    copy: true)
else
  shaderc_dep = cc.find_library('shaderc', 
	dirs: [meson.source_root() / 'thirdparty/shaderc/linux'],
	static: true)
endif
