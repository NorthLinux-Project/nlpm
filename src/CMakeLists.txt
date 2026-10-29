add_library(nlpm_core STATIC
  util/hash_map.c
  util/version.c
  crypto/sha256.c
  ../third_party/toml-c/toml.c
  config/config.c
  db/db_sync.c
  db/db_local.c
  pkg/pkg_read.c
  pkg/pkg_build.c
  pkg/pkg_install.c
  pkg/pkg_remove.c
  pkg/pkg_upgrade.c
  net/net_download.c
  repo/repo_index.c
  repo/repo_sync.c
  solver/term.c
  solver/incompat.c
  solver/solver.c
  cmd/cmd_sync.c
  cmd/cmd_install.c
  cmd/cmd_remove.c
  cmd/cmd_upgrade.c
  cmd/cmd_search.c
  cmd/cmd_info.c
  cmd/cmd_list.c
  cmd/cmd_owns.c
  cmd/cmd_build.c
)

target_compile_features(nlpm_core PUBLIC c_std_11)
target_include_directories(nlpm_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/include
  ${CMAKE_SOURCE_DIR}/third_party/toml-c
)

target_link_libraries(nlpm_core
  PUBLIC
    system_deps
  PRIVATE
    project_warnings
)

add_executable(nlpm main.c)
target_link_libraries(nlpm
  PRIVATE
    nlpm_core
    project_warnings
)
