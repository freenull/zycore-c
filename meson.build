project(
  'zycore',
  'c',
  version: '1.1.0.0',
  license: 'MIT'
)

includes = include_directories('include')
subdir('src')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  zycore_lib,
  version: '1.1.0.0',
  name: 'libzycore',
  filebase: 'zycore',
  description: 'Zycore'
)
