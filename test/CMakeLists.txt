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

add_executable ( check_spudlib ${test_srcs} )
target_link_libraries ( check_spudlib PRIVATE spud )
target_link_libraries ( check_spudlib PRIVATE ${CHECK_LIBRARIES} )
target_include_directories ( check_spudlib PRIVATE ../include )
target_include_directories ( check_spudlib PRIVATE ${CHECK_INCLUDE_DIRS} )
add_definitions(-DUSE_CBOR_CONTEXT)

add_custom_target ( check_spudlibrun ALL
    COMMAND ./check_spudlib
    DEPENDS check_spudlib
    WORKING_DIRECTORY ${dist_dir}/bin
  )
