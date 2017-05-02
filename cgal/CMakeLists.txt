# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.


project( skeletons )

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

if ( COMMAND cmake_policy )
  cmake_policy( SET CMP0003 NEW )
endif()


find_package(CGAL QUIET COMPONENTS Core )

#FIXME this works but needs two cmake runs to take effect :/
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


if ( CGAL_FOUND )

  include( ${CGAL_USE_FILE} )

  include( CGAL_CreateSingleSourceCGALProgram )

  include_directories (BEFORE ../../include)

  create_single_source_cgal_program( "skeleton-tgf.cpp" )

else()

    message(STATUS "This program requires the CGAL library, and will not be compiled.")

endif()
