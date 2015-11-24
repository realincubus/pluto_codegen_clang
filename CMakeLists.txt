CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(pluto_codegen_clang)

SET( CMAKE_BUILD_TYPE DEBUG )

#SET( CMAKE_C_COMPILER clang++ )
#SET( CMAKE_CXX_COMPILER clang++ )
SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-exceptions -fno-rtti -fPIC -fvisibility-inlines-hidden -fno-omit-frame-pointer -DCLOOG_INT_GMP=1" )
SET( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -DGLIBCXX_DEBUG" )
SET( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -std=c++1y" )
SET( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}" )
SET( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" )

#SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )
#SET( LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib )

add_subdirectory( clast-clang )
include_directories( clast-clang )
include_directories( ../pluto_src/src )
include_directories( ../pluto_src/piplib/include )

include_directories( ../clan/osl/include )

SET( CMAKE_EXPORT_COMPILE_COMMANDS on )

#INCLUDE_DIRECTORIES( )
ADD_LIBRARY( pluto_codegen_clang SHARED main.cpp)

TARGET_LINK_LIBRARIES( pluto_codegen_clang plutocore )

