project(platform_abstraction C)

set(platform_sources ${CMAKE_CURRENT_SOURCE_DIR}/task.c)
add_library(platform_abstraction STATIC ${platform_sources})
target_link_libraries(platform_abstraction PUBLIC freertos_kernel freertos_config)
target_include_directories(platform_abstraction PUBLIC ${API_INCLUDES})
target_compile_features(platform_abstraction PUBLIC c_std_11)
