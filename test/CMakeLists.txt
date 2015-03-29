#
#
# Compiling/running tests

add_definitions(-DUSE_CBOR_CONTEXT)
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/test )

function (ls_test name)
  add_executable ( ${name}_test ${name}_test.c test_utils.c )
  target_compile_options( ${name}_test PRIVATE "-pthread" )
  target_link_libraries ( ${name}_test PRIVATE spud "pthread" "m")
  target_include_directories ( ${name}_test PRIVATE ../include )
  add_test ( NAME ${name} COMMAND ${name}_test )
endfunction()

ls_test ( cbor )
ls_test ( ls_error )
ls_test ( ls_eventing )
ls_test ( ls_htable )
ls_test ( ls_log )
ls_test ( ls_mem )
ls_test ( ls_sockaddr )
ls_test ( ls_str )
ls_test ( spud )
ls_test ( tube )

include ( CTest )
