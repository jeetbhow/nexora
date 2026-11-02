lib_sources = files(
    'app.cpp',
    'server/server.cpp',
    'http/headers.cpp',
    'http/http.cpp',
    'http/request.cpp',
    'http/response.cpp',
    'utils.cpp',
    'router/router.cpp',
    'router/node.cpp'
)

json_dep = dependency('nlohmann_json')

nexora_lib = static_library(
  'nexora',
  lib_sources,
  include_directories: project_inc,
  dependencies: json_dep,
)

nexora_dep = declare_dependency(
  link_with: nexora_lib,
  include_directories: project_inc,
  dependencies: json_dep,
)
