function(add_unity_test test_target test_source)
  add_executable(${test_target} ${test_source})
  target_link_libraries(${test_target}
    PRIVATE
      nlpm_core
      unity
      project_warnings
  )
  target_include_directories(${test_target}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src
  )
  add_test(NAME ${test_target} COMMAND ${test_target})
endfunction()

add_unity_test(test_version unit/test_version.c)
add_unity_test(test_sha256 unit/test_sha256.c)
add_unity_test(test_collections unit/test_collections.c)
add_unity_test(test_config unit/test_config.c)
add_unity_test(test_db unit/test_db.c)
add_unity_test(test_pkg unit/test_pkg.c)
add_unity_test(test_repo unit/test_repo.c)
add_unity_test(test_solver unit/test_solver.c)
add_unity_test(test_install unit/test_install.c)
add_unity_test(test_pipeline integration/test_pipeline.c)
