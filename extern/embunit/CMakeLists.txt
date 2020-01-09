project(embunit C)


enable_testing()

file(GLOB SOURCES embUnit/*.c embUnit/*.h)
file(GLOB TEXTUI_SOURCES textui/*.c textui/*.h)
add_library(textui ${TEXTUI_SOURCES})
target_compile_features(textui PRIVATE c_std_11)
target_include_directories(textui PUBLIC ${PROJECT_SOURCE_DIR})
#get_target_property(TEXTUI_INCLUDES textui INCLUDE_DIRECTORIES)
#message("Includes for textui")
#print_list("${TEXTUI_INCLUDES}")

add_library(embunit ${SOURCES})
target_compile_features(embunit PRIVATE c_std_11)
target_link_libraries(embunit textui)
target_include_directories(embunit PUBLIC ${PROJECT_SOURCE_DIR})
#get_target_property(EMBUNIT_INCLUDES embunit INCLUDE_DIRECTORIES)
#message("Includes for embunit")
#print_list("${EMBUNIT_INCLUDES}")



