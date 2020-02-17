physx_include_dirs = [
  include_directories('physx'),
  include_directories('pxshared')
]

physx_deps = []

if target_machine.system() == 'windows'
  if get_option('debug')
    physx_lib_dir = meson.current_source_dir() / 'windows/debug'
  else
    physx_lib_dir = meson.current_source_dir() / 'windows/release'
  endif
  physx_deps += cc.find_library('PhysX_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXCommon_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXFoundation_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXExtensions_static_64', dirs: [physx_lib_dir])
else
  physx_lib_dir = meson.current_source_dir() / 'linux'
  physx_deps += cc.find_library('PhysX_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXCommon_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXFoundation_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXExtensions_static_64', dirs: [physx_lib_dir])
endif
