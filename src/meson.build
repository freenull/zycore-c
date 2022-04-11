zycore_sources = [
  'API/Memory.c',
  'API/Process.c',
  'API/Synchronization.c',
  'API/Terminal.c',
  'API/Thread.c',
  'Allocator.c',
  'ArgParse.c',
  'Bitset.c',
  'Format.c',
  'List.c',
  'String.c',
  'Vector.c',
  'Zycore.c',
]

zycore_lib = library(
  'zycore',
  zycore_sources,
  include_directories: includes,
  install: true
)

zycore_lib_dep = declare_dependency(
  include_directories: includes,
  link_with: zycore_lib
)

