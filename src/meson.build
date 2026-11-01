lib_sources = files(
    'app.cpp',
    'http/headers.cpp',
    'http/http.cpp',
    'utils.cpp',
    'router/router.cpp',
    'router/node.cpp'
)

nexora_lib = static_library(
  'nexora',
  lib_sources,
  include_directories: project_inc,
)

nexora_dep = declare_dependency(
  link_with: nexora_lib,
  include_directories: project_inc,
)
