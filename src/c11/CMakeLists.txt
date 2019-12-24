project(platform_abstraction C)
find_package(Threads REQUIRED)

file(GLOB PLATFORM_HEADER_LIST CONFIGURE_DEPENDS "${cutils_SOURCE_DIR}/inc/cutils/c11/*.h" )

set(PLATFORM_SOURCES task.c event_flag.c logger.c)
add_library(platform_abstraction STATIC "${PLATFORM_HEADER_LIST}" "${HEADER_LIST}" "${PLATFORM_SOURCES}" )
set_target_properties(platform_abstraction PROPERTIES LINKER_LANGUAGE C)
target_compile_features(platform_abstraction PUBLIC c_std_11)
target_link_libraries(platform_abstraction ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(platform_abstraction PUBLIC ${API_INCLUDES})
get_target_property(TARGET_SOURCES platform_abstraction SOURCES)
message("Sources")
print_list("${TARGET_SOURCES}")


