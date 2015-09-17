#
#
# compiling/installing sources for SPUDlib

set ( spud_srcs
      ls_error.c
      ls_eventing.c
      ls_htable.c
      ls_log.c
      ls_mem.c
      ls_pktinfo.c
      ls_queue.c
      ls_sockaddr.c
      ls_str.c
      ls_timer.c
      spud.c
      tube.c
      tube_manager.c
      tube_stream.c
      map.c
)

set ( spud_headers
      ls_eventing.h
      ls_eventing_int.h
      ls_log_int.h
      ls_pool_types.h
      ls_queue.h
      ls_str.h
      tube_manager_int.h
)

set (spud_all ${spud_srcs} ${spud_headers})
UncrustifyDir(spud_all)

add_definitions(-DUSE_CBOR_CONTEXT)
add_library ( spud SHARED ${spud_srcs} )
target_include_directories ( spud PUBLIC ../include )
target_include_directories ( spud PRIVATE ../src )
target_link_libraries ( spud PRIVATE cn-cbor )

install ( TARGETS spud
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
          RUNTIME DESTINATION bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
if (coveralls)
    include(Coveralls)
    coveralls_turn_on_coverage()

    set(COVERAGE_SRCS "")
    foreach (S ${spud_srcs})
      get_filename_component(S_ABS ${S} ABSOLUTE)
      list (APPEND COVERAGE_SRCS ${S_ABS})
    endforeach()

    # Create the coveralls target.
    coveralls_setup(
        "${COVERAGE_SRCS}"
        ${coveralls_send}                 # If we should upload.
    )

    #add_dependencies(coveralls, all)
endif()
