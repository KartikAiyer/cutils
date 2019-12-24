enable_language( C )
file( GLOB HEADER_LIST CONFIGURE_DEPENDS "${cutils_SOURCE_DIR}/inc/cutils/*.h" )

#Configure the platform specific stuff
#Currently only C11 APIs supported
set( CUTILS_PLATFORM_TYPE c11 )
set( TEMPLATE_HEADER_LOCATION ${cutils_SOURCE_DIR}/inc )
message( ${TEMPLATE_HEADER_LOCATION} )

configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/mutex.h.in ${PROJECT_BINARY_DIR}/inc/cutils/mutex.h )
configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/task.h.in ${PROJECT_BINARY_DIR}/inc/cutils/task.h )
configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/event_flag.h.in ${PROJECT_BINARY_DIR}/inc/cutils/event_flag.h )

function( print_list theList )
  foreach( item ${theList} )
    message("${item}")
  endforeach()
endfunction()
# set up options
option( USE_STD_PRTINTF "Use the std lib provided printf function for debug output" ON )
if( "${USE_STD_PRINTF}" )
  add_definitions( -DUSE_STD_PRINTF )
endif()

if( "${CUTILS_PLATFORM_TYPE}" STREQUAL "c11" )
  add_subdirectory( c11 )
else()
  message( FATAL_ERROR "Need a proper cutils platform type" )
endif()

set( SOURCES ring_buffer.c )
add_library( cutils ${SOURCES} ${HEADER_LIST} )
target_include_directories( cutils PUBLIC ${API_INCLUDES} )
target_compile_features( cutils PUBLIC c_std_11 )
target_link_libraries( cutils platform_abstraction )
get_target_property( TARGET_SOURCES cutils SOURCES )

source_group( TREE "${PROJECT_SOURCE_DIR}/inc" PREFIX "Header Files" FILES ${HEADER_LIST} )

