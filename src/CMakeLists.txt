#
#
# compiling/installing sources for SPUDlib

set ( spud_srcs
      cn-cbor/cn-cbor.c
      cn-cbor/cbor.h
      cn-cbor/cn-cbor.c
      cn-cbor/cn-create.c
      cn-cbor/cn-encoder.c
      cn-cbor/cn-encoder.h
      cn-cbor/cn-error.c
      cn-cbor/cn-manip.c
      ls_error.c
      ls_eventing.c
      ls_eventing.h
      ls_htable.c
      ls_log.c
      ls_mem.c
      ls_sockaddr.c
      ls_str.c
      spud.c
      tube.c
)

add_library ( spud SHARED ${spud_srcs} )
target_include_directories ( spud PUBLIC ../include )
target_include_directories ( spud PRIVATE ../src )

install ( TARGETS spud
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
          RUNTIME DESTINATION bin)
