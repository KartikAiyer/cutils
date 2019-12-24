file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${cutils_SOURCE_DIR}/inc/cutils/*.h" )

set( SOURCES library.c )
add_library( cutils ${SOURCES} ${HEADER_LIST})
target_include_directories(cutils PUBLIC ../inc)
target_compile_features(cutils PUBLIC c_std_11)

source_group(TREE "${PROJECT_SOURCE_DIR}/inc" PREFIX "Header Files" FILES ${HEADER_LIST})
