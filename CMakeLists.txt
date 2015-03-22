#
#
# top level build file for the SPUDlib

## prepare CMAKE
cmake_minimum_required ( VERSION 3.0.0 )
project ( "spudlib" )
set ( CPACK_PROJECT_VERSION_MAJOR "0" )
set ( CPACK_PROJECT_VERSION_MINOR "3pre" )

find_package ( PkgConfig )

set ( package_prefix
      ${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE} )

include ( CheckIncludeFiles )
include ( CheckLibraryExists )
include ( CheckFunctionExists ) 
include ( CheckTypeSize )

set ( dist_dir ${CMAKE_BINARY_DIR}/dist )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/bin )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dist_dir}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dist_dir}/lib )

## check and generate configuration
pkg_check_modules ( CHECK check>=0.9.8 )

check_include_files ( stdint.h HAVE_STDINT_H )
check_include_files ( stdlib.h HAVE_STDLIB_H )
check_include_files ( stdbool.h HAVE_STDBOOL_H )
check_include_files ( stdint.h HAVE_STDINT_H )
check_function_exists ( memset HAVE_MEMSET )
check_function_exists ( arc4random HAVE_ARC4RANDOM )
check_library_exists ( pthread pthread_create "" HAVE_LIBPTHREAD )
check_library_exists ( m tan "" HAVE_LIBM )

## check for urandom
if ( EXISTS "/dev/urandom" )
  set ( HAVE__DEV_URANDOM 1 CACHE INTERNAL "/dev/urandom exists" )
  message ( STATUS "Looking for /dev/urandom - found" )
elseif ()
  message ( STATUS "Looking for /dev/urandom - NOT FOUND" )
endif ()

configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

## setup global includes
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

## include the parts
add_subdirectory ( include )
add_subdirectory ( src )
add_subdirectory ( test )

## setup packaging
set ( CPACK_INCLUDE_TOPLEVEL_DIRECTORY "1" )
set ( CPACK_PROJECT_VERSION_MINOR "3pre" )
set ( CPACK_GENERATOR "TGZ" )
set ( CPACK_SOURCE_GENERATOR "TGZ" )
set ( CPACK_SOURCE_IGNORE_FILES "/.git/;${CMAKE_CURRENT_BINARY_DIR}" )
include ( CPack )
