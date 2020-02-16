physx_include_dirs = [
  include_directories('physx'),
  include_directories('pxshared')
]

physx_deps = []

if target_machine.system() == 'windows'
  physx_lib_dir = meson.current_source_dir() / 'windows'
  # physx_deps += cc.find_library('PhysXDevice64', dirs: [physx_lib_dir])
  # configure_file(input: 'windows/PhysXDevice64.dll', output: 'PhysXDevice64.dll', copy: true)
else
  physx_lib_dir = meson.current_source_dir() / 'linux'
  physx_deps += cc.find_library('PhysX_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXCommon_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXFoundation_static_64', dirs: [physx_lib_dir])
  physx_deps += cc.find_library('PhysXExtensions_static_64', dirs: [physx_lib_dir])
endif
