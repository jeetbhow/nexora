lib_sources = files(
    'http/headers.cpp',
    'http/http.cpp',
    'utils.cpp',
    'router/router.cpp',
    'router/node.cpp'
)

http_lib = static_library(
  'http',
  lib_sources,
  include_directories: project_inc,
)

http_dep = declare_dependency(
  link_with: http_lib,
  include_directories: project_inc,
)
