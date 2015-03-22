#
#
# Compiling/running tests

set ( test_srcs
      cbor_test.c
      ls_error_test.c
      ls_eventing_test.c
      ls_htable_test.c
      ls_log_test.c
      ls_mem_test.c
      ls_sockaddr_test.c
      ls_str_test.c
      spud_test.c
      test_utils.c
      test_utils.h
      testmain.c
      tube_test.c )
      
link_directories ( ${CHECK_LIBRARY_DIRS} )

add_executable ( spud-test ${test_srcs} )
target_link_libraries ( spud-test PRIVATE spud )
target_link_libraries ( spud-test PRIVATE ${CHECK_LIBRARIES} )
target_include_directories ( spud-test PRIVATE ../include )
target_include_directories ( spud-test PRIVATE ${CHECK_INCLUDE_DIRS} )

add_custom_target ( spud-testrun ALL
    COMMAND ./spud-test
    DEPENDS spud-test
    WORKING_DIRECTORY ${dist_dir}/bin
  )
