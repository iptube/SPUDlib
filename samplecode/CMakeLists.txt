#
#
# Sample programs

add_executable ( spudtest spudtest.c )
target_link_libraries ( spudtest PRIVATE spud cn-cbor pthread )

add_executable ( spudecho spudecho.c )
target_link_libraries ( spudecho PRIVATE spud cn-cbor )

add_executable ( spudload spudload.c gauss.c )
target_link_libraries ( spudload PRIVATE spud cn-cbor pthread m )

add_definitions(-DUSE_CBOR_CONTEXT)
include_directories ( ../include )
link_directories ( ${CHECK_LIBRARY_DIRS} )
