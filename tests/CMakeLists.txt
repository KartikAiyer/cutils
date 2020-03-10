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

package_add_test(all_tests queue_tests.cpp os_test.cpp mem_ring_buffer_test.cpp klist_test.cpp)
target_compile_features(all_tests PRIVATE cxx_std_17 c_std_11)
target_include_directories(all_tests PRIVATE "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(all_tests cutils)

add_executable(pool_tests pool_tests.c)
target_include_directories(pool_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(pool_tests cutils embunit)
target_compile_features(pool_tests PRIVATE c_std_11)

add_executable(dispatch_queue_tests dispatch_queue_tests.c)
target_include_directories(dispatch_queue_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(dispatch_queue_tests cutils embunit)
target_compile_features(dispatch_queue_tests PRIVATE c_std_11)


# The following unit test uses Malloc and so will only be available on platforms that support malloc.h
CHECK_INCLUDE_FILE(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
if(HAVE_MALLOC_H)
if(HAVE_STDLIB_H)
  add_executable(notifier_tests notifier_tests.c)
  target_include_directories(notifier_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
  target_link_libraries(notifier_tests cutils embunit)
  target_compile_features(notifier_tests PRIVATE c_std_11)
endif()
endif()

add_executable(accumulator_tests accumulator_tests.c)
target_include_directories(accumulator_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(accumulator_tests cutils embunit)
target_compile_features(accumulator_tests PRIVATE c_std_11)

add_executable(asyncio_tests asyncio_test.c)
target_include_directories(asyncio_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(asyncio_tests cutils embunit)
target_compile_features(asyncio_tests PRIVATE c_std_11)

add_executable(state_event_loop_tests state_event_loop_tests.c)
target_include_directories(state_event_loop_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${LIBRARY_INCLUDE_DIR}")
target_link_libraries(state_event_loop_tests cutils embunit)
target_compile_features(state_event_loop_tests PRIVATE c_std_11)
