#
#
# top level build file for the SPUDlib

## prepare CMAKE
cmake_minimum_required ( VERSION 3.2.0 )

set ( SPUDLIB_VERSION_MAJOR "0" CACHE STRING "Project major version number")
set ( SPUDLIB_VERSION_MINOR "4" CACHE STRING "Project minor version number" )
set ( SPUDLIB_VERSION_PATCH "0" CACHE STRING "Project patch version number" )
mark_as_advanced(SPUDLIB_VERSION_MAJOR SPUDLIB_VERSION_MINOR SPUDLIB_VERSION_PATCH)

set ( SPUDLIB_VERSION "${SPUDLIB_VERSION_MAJOR}.${SPUDLIB_VERSION_MINOR}.${SPUDLIB_VERSION_PATCH}" )

project ( "spudlib" VERSION "${SPUDLIB_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include ( Uncrustify )

find_package(Doxygen)

## setup options
option ( verbose "Produce verbose makefile output" OFF )
option ( optimize "Set high optimization level" OFF )
option ( fatal_warnings "Treat build warnings as errors" ON )
option ( coveralls "Generate coveralls data" ON )
option ( coveralls_send "Send data to coveralls site" OFF )
option ( build_docs "Create docs using Doxygen" ${DOXYGEN_FOUND} )
option ( uncrustify "Uncrustify the source code" ${UNCRUSTIFY_FOUND} )

set ( dist_dir    ${CMAKE_BINARY_DIR}/dist )
set ( prefix      ${CMAKE_INSTALL_PREFIX} )
set ( exec_prefix ${CMAKE_INSTALL_PREFIX}/bin )
set ( libdir      ${CMAKE_INSTALL_PREFIX}/lib )
set ( includedir  ${CMAKE_INSTALL_PREFIX}/include )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/spudlib.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/spudlib.pc @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/spudlib.pc DESTINATION lib/pkgconfig )

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
  add_definitions ( -std=gnu99 -Wall -Wextra -pedantic )
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

find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )

install (FILES AUTHORS LICENSE README.md DESTINATION .)

## setup packaging
set ( CPACK_GENERATOR "TGZ" )
set ( CPACK_PACKAGE_VERSION "${PROJECT_VERSION}" )
set ( CPACK_SOURCE_GENERATOR "TGZ" )
set ( CPACK_SOURCE_IGNORE_FILES "/\\\\.git/" )
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.gitignore igs)
foreach (ig IN ITEMS ${igs})
    # remove comments
    string ( REGEX REPLACE "^\\s*#.*" "" ig "${ig}")
    # remove any other whitespace
    string ( STRIP "${ig}" ig)
    # anything left?
    if (ig)
      # dots are literal
      string ( REPLACE "." "\\\\." ig "${ig}" )
      # stars are on thars
      string ( REPLACE "*" ".*" ig "${ig}" )
      list ( APPEND CPACK_SOURCE_IGNORE_FILES "/${ig}/" )
    endif()
endforeach()

#message ( "CPACK_SOURCE_IGNORE_FILES: " ${CPACK_SOURCE_IGNORE_FILES} )
set ( CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.md )
set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )

include ( CPack )
include ( CTest )
include ( LCov )

if (build_docs)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
endif()

UncrustifyTop(${uncrustify})

Include(ExternalProject)
ExternalProject_Add(
   project_cn-cbor
   GIT_REPOSITORY https://github.com/hildjj/cn-cbor.git
   CMAKE_ARGS -Doptimize=OFF -Duse_context=ON -Dbuild_docs=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
   INSTALL_DIR "${dist_dir}"
   UPDATE_DISCONNECTED 1
)

ExternalProject_Get_Property(project_cn-cbor install_dir)
include_directories ( "${install_dir}/include" )
add_library(cn-cbor STATIC IMPORTED)
set_property(TARGET cn-cbor PROPERTY IMPORTED_LOCATION "${install_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}cn-cbor${CMAKE_SHARED_LIBRARY_SUFFIX}")
add_dependencies(cn-cbor project_cn-cbor)

## include the parts
add_subdirectory ( include )
add_subdirectory ( src )
add_subdirectory ( test )
add_subdirectory ( samplecode )
