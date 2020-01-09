enable_language(CXX C)

include(TestUtil)
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/embunit" "extern/embunit")

set(LIBRARY_INCLUDE_DIR "${API_INCLUDES}")
package_add_test(klist_test klist_test.cpp)
target_compile_features(klist_test PRIVATE cxx_std_17)
target_include_directories(klist_test PRIVATE "${LIBRARY_INCLUDE_DIR}")

package_add_test(mem_ring_buf_test mem_ring_buffer_test.cpp)
target_compile_features(mem_ring_buf_test PRIVATE cxx_std_17)
target_include_directories(mem_ring_buf_test PRIVATE "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(mem_ring_buf_test cutils)

package_add_test( os_test os_test.cpp )
target_compile_features(os_test PRIVATE cxx_std_17 c_std_11)
target_include_directories(os_test PRIVATE "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(os_test cutils)

package_add_test( queue_tests queue_tests.cpp)
target_compile_features(queue_tests PRIVATE cxx_std_17 c_std_11)
target_include_directories(queue_tests PRIVATE "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(queue_tests cutils)
get_target_property(TARGET_INCLUDES queue_tests INCLUDE_DIRECTORIES)
message("Includes")
print_list("${TARGET_INCLUDES}")

package_add_test(all_tests queue_tests.cpp os_test.cpp mem_ring_buffer_test.cpp klist_test.cpp)
target_compile_features(all_tests PRIVATE cxx_std_17 c_std_11)
target_include_directories(all_tests PRIVATE "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(all_tests cutils)

add_executable(pool_tests pool_tests.c)
target_include_directories(pool_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(pool_tests cutils embunit)
target_compile_features(pool_tests PRIVATE c_std_11)


