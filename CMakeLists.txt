#
#
# top level build file for the SPUDlib

## prepare CMAKE
cmake_minimum_required ( VERSION 3.0.0 )
project ( "spudlib" VERSION "0.3.0" )

## setup options
option ( verbose "Produce verbose makefile output" OFF )
option ( optimize "Set high optimization level" OFF )
option ( fatal_warnings "Treat build warnings as errors" ON )

## setup CMAKE building
set ( CPACK_PROJECT_VERSION_MAJOR "0" )
set ( CPACK_PROJECT_VERSION_MINOR "3pre" )
find_package ( PkgConfig )

set ( dist_dir ${CMAKE_BINARY_DIR}/dist )

set ( package_prefix "${CMAKE_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}" )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/bin )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dist_dir}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dist_dir}/lib )
set ( CMAKE_BUILD_TYPE Debug )

## check and generate configuration

include ( CheckIncludeFiles )
include ( CheckLibraryExists )
include ( CheckFunctionExists )
include ( CheckTypeSize )

pkg_check_modules ( CHECK check>=0.9.8 )

check_include_files ( stdint.h HAVE_STDINT_H )
check_include_files ( stdlib.h HAVE_STDLIB_H )
check_include_files ( stdbool.h HAVE_STDBOOL_H )
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

## setup global compiler options
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
if ( CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_C_COMPILER_ID MATCHES "Clang" )
  message ( STATUS "adding GCC/Clang options ")
  add_definitions ( -g -std=gnu99 -Wall -Wextra -pedantic )
  ## disable VLA "is a GNU extension" warning
  add_definitions ( -Wno-gnu-zero-variadic-macro-arguments )
  if ( fatal_warnings )
    add_definitions ( -Werror )
  endif ()
  if ( optimize )
    add_definitions ( -O2 )
  endif ()
elseif ( MSVC )
  add_definitions ( /W3 )
  if ( fatal_warnings )
    add_definitions ( /WX )
  endif ()
else ()
  message ( FATAL_ERROR "unhandled compiler id: ${CMAKE_C_COMPILER_ID}" )
endif ()
if ( verbose )
  set ( CMAKE_VERBOSE_MAKEFILE ON )
endif ()

## include the parts
add_subdirectory ( include )
add_subdirectory ( src )
add_subdirectory ( test )

## setup packaging
set ( CPACK_GENERATOR "TGZ" )
set ( CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" )
set ( CPACK_SOURCE_GENERATOR "TGZ" )
set ( CPACK_SOURCE_IGNORE_FILES "/.git/;${CMAKE_CURRENT_BINARY_DIR}" )
include ( CPack )
