cc = meson.get_compiler('c')

glfw_core_src = [
  'src/context.c',
  'src/init.c',
  'src/input.c',
  'src/monitor.c',
  'src/vulkan.c',
  'src/window.c'
]

glfw_platform_src = []
glfw_deps = []
glfw_deps += cc.find_library('m', required: false)
glfw_deps += cc.find_library('dl', required: false)
glfw_deps += dependency('threads')

conf_data = configuration_data()

if target_machine.system() == 'linux'
    x11_dep = dependency('x11')
    x11_xcb_dep = dependency('x11-xcb')
    xrandr_dep = dependency('xrandr')
    xinerama_dep = dependency('xinerama')
    xkb_dep = dependency('xkbcommon')
    xcursor_dep = dependency('xcursor')
    glfw_deps += [x11_dep, xrandr_dep, xinerama_dep, xkb_dep, xcursor_dep, x11_xcb_dep]
    glfw_platform_src += [
	  'src/x11_init.c',
	  'src/x11_monitor.c',
	  'src/x11_window.c',
	  'src/xkb_unicode.c',
	  'src/posix_time.c',
	  'src/posix_thread.c',
	  'src/glx_context.c',
	  'src/egl_context.c',
	  'src/osmesa_context.c',

	  'src/linux_joystick.c'
    ]

    conf_data.set('_GLFW_X11', 1)
elif target_machine.system() == 'windows'
    gdi_dep = cc.find_library('gdi32')

    glfw_deps += [gdi_dep]
    glfw_platform_src += [
	  'src/win32_init.c',
	  'src/win32_joystick.c',
	  'src/win32_monitor.c',
	  'src/win32_time.c',
	  'src/win32_thread.c',
	  'src/win32_window.c',
	  'src/wgl_context.c',
	  'src/egl_context.c',
	  'src/osmesa_context.c',
    ]

    conf_data.set('_GLFW_WIN32', 1)
else
   error('OS not supported')
endif

conf_file = configure_file(configuration: conf_data, output: 'glfw_config.h')

glfw_lib = static_library('glfw', glfw_core_src + glfw_platform_src,
  include_directories: ['src', 'include'],
  dependencies: glfw_deps,
  c_args: ['-D_GLFW_USE_CONFIG_H'])

glfw_dep = declare_dependency(
  include_directories: ['include'],
  link_with: glfw_lib)
