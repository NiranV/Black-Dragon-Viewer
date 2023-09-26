# -*- cmake -*-
cmake_minimum_required( VERSION 3.13 FATAL_ERROR )

include( Prebuilt )
include_guard()

add_library( ll::SDL2 INTERFACE IMPORTED )

use_system_binary( SDL2 )
use_prebuilt_binary( SDL2 )

find_library( SDL2_LIBRARY
    NAMES SDL2
    PATHS "${LIBS_PREBUILT_DIR}/lib/release")
if ( "${SDL2_LIBRARY}" STREQUAL "SDL2_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "unable to find SDL2_LIBRARY" )
endif()

target_link_libraries( ll::SDL2 INTERFACE "${SDL2_LIBRARY}" )
target_include_directories( ll::SDL2 SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include" )

