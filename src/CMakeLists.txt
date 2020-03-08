enable_language( C )
file( GLOB HEADER_LIST CONFIGURE_DEPENDS "${cutils_SOURCE_DIR}/inc/cutils/*.h" )

#Configure the platform specific stuff
#Currently only C11 APIs supported
set( CUTILS_PLATFORM_TYPE pthread )
set( TEMPLATE_HEADER_LOCATION ${cutils_SOURCE_DIR}/inc )

configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/mutex.h.in ${PROJECT_BINARY_DIR}/inc/cutils/mutex.h )
configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/task.h.in ${PROJECT_BINARY_DIR}/inc/cutils/task.h )
configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/event_flag.h.in ${PROJECT_BINARY_DIR}/inc/cutils/event_flag.h )
configure_file( ${TEMPLATE_HEADER_LOCATION}/cutils/ts_queue.h.in ${PROJECT_BINARY_DIR}/inc/cutils/ts_queue.h)

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

# Build the logger common stuff.
add_library( logger_basic logger.c)
target_compile_definitions(logger_basic PUBLIC c_std_11)
target_include_directories(logger_basic PUBLIC ${API_INCLUDES})

if( "${CUTILS_PLATFORM_TYPE}" STREQUAL "c11" )
  add_subdirectory( c11 )
elseif("${CUTILS_PLATFORM_TYPE}" STREQUAL "pthread")
  add_subdirectory(pthread)
else()
  message( FATAL_ERROR "Need a proper cutils platform type" )
endif()

set( SOURCES ring_buffer.c dispatch_queue.c )
add_library( cutils ${SOURCES} ${HEADER_LIST} )
target_include_directories( cutils PUBLIC ${API_INCLUDES} )
target_compile_features( cutils PUBLIC c_std_11 )
target_link_libraries( cutils logger_basic platform_abstraction )

source_group( TREE "${PROJECT_SOURCE_DIR}/inc" PREFIX "Header Files" FILES ${HEADER_LIST} )

