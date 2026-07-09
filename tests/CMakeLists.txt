# Tests use pure C (C11). No CXX is needed anymore.

include("${PROJECT_SOURCE_DIR}/cmake/EmbUnitTest.cmake")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/embunit" "extern/embunit")

set(API_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/inc" "${CMAKE_CURRENT_BINARY_DIR}/inc")

# Hosted-platform suites (pthread/c11). These assume a hosted libc + crt0 runner and do not build under the freertos
# bare-metal cross toolchain.
if(NOT CUTILS_PLATFORM_TYPE STREQUAL freertos)

  # --- Migrated suites (now pure C on embunit) ---
  package_add_embunit_test(NAME klist_test FILES klist_test.c)
  package_add_embunit_test(NAME mem_ring_buf_test FILES mem_ring_buffer_test.c LIBS cutils)
  package_add_embunit_test(NAME os_test FILES os_test.c LIBS cutils)
  package_add_embunit_test(NAME queue_tests FILES queue_test.c LIBS cutils)

  # --- Existing embunit suites ---
  package_add_embunit_test(NAME pool_tests FILES pool_tests.c)
  package_add_embunit_test(NAME dispatch_queue_tests FILES dispatch_queue_tests.c)

  package_add_embunit_test(NAME accumulator_tests FILES accumulator_tests.c)
  package_add_embunit_test(NAME asyncio_tests FILES asyncio_test.c)
  package_add_embunit_test(NAME state_event_loop_tests FILES state_event_loop_tests.c)

  # --- Aggregate test binary ---
  # Only includes suites that run reliably on the host platform. dispatch_queue, asyncio, and state_event_loop are
  # excluded because they depend on task/thread infrastructure unavailable on pthread host.
  add_executable(
    all_embunit_tests
    all_embunit_tests.c
    klist_test.c
    mem_ring_buffer_test.c
    os_test.c
    queue_test.c
    pool_tests.c
    accumulator_tests.c
    notifier_tests.c
  )
  target_include_directories(all_embunit_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit" "${API_INCLUDE_DIR}")
  target_compile_definitions(all_embunit_tests PRIVATE AGGREGATE_RUNNER)
  target_link_libraries(all_embunit_tests cutils embunit)
  target_compile_features(all_embunit_tests PRIVATE c_std_11)
  add_test(NAME all_embunit_tests COMMAND all_embunit_tests)
  set_target_properties(all_embunit_tests PROPERTIES FOLDER tests)
else()
  add_subdirectory(freertos/GCC_ARM_CM7)
endif()
