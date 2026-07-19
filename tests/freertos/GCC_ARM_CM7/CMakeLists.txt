# embUnit test runners for the FreeRTOS/AN500 (QEMU mps2-an500, Cortex-M7) target.
#
# Registers one ctest per suite (embtest_<suite>) plus an opt-in all-in-one
# embtest_runner (CUTILS_FREERTOS_AGGREGATE_RUNNER=ON). Each runner is a
# bare-metal ELF that boots FreeRTOS under QEMU; semihost_exit(0|1) in
# retarget.c becomes QEMU's process exit code, which ctest reads as pass/fail.
#
# Design: all suite sources + the bare-metal scaffolding compile once into the
# embtest_common OBJECT library; each runner adds only its main.c (compiled
# with -DCUTILS_EMBTEST_SUITE=<accessor> to select one suite, or without it
# for the aggregate). -ffunction-sections + --gc-sections (see
# cmake/arm-none-eabi.cmake) then strip every unreferenced suite, so each
# per-suite ELF carries only its own suite + shared bare-metal.

# --- shared bare-metal scaffolding + every suite source (compiled once) ------
file(GLOB test_sources ${PROJECT_SOURCE_DIR}/tests/*.c)
list(REMOVE_ITEM test_sources ${PROJECT_SOURCE_DIR}/tests/all_embunit_tests.c)

add_library(
  embtest_common
  OBJECT
  ${test_sources}
  startup.c
  retarget.c)
target_include_directories(
  embtest_common
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/extern/embunit ${API_INCLUDES})
# AGGREGATE_RUNNER suppresses each suite's hosted #ifndef AGGREGATE_RUNNER
# main() so the bare-metal main() in main.c is the sole entry point.
target_compile_definitions(embtest_common PUBLIC AGGREGATE_RUNNER)
target_link_libraries(embtest_common PUBLIC cutils platform_abstraction freertos_kernel embunit)

# --- QEMU probe (once) -------------------------------------------------------
find_program(QEMU_SYSTEM_ARM qemu-system-arm)

# --- bare-metal link flags shared by every runner ----------------------------
set(embtest_link_flags -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostartfiles)

# --- helper: build one per-suite runner ELF + register a ctest ---------------
# freertos_add_embunit_test(NAME <suite> SUITE_FN <accessor> [TIMEOUT <sec>])
function(freertos_add_embunit_test)
  cmake_parse_arguments(FAET "" "NAME;SUITE_FN;TIMEOUT" "" ${ARGN})
  if(NOT FAET_NAME)
    message(FATAL_ERROR "freertos_add_embunit_test requires NAME")
  endif()
  if(NOT FAET_SUITE_FN)
    message(FATAL_ERROR "freertos_add_embunit_test requires SUITE_FN")
  endif()
  if(NOT FAET_TIMEOUT)
    set(FAET_TIMEOUT 60)
  endif()

  set(target embtest_${FAET_NAME})
  add_executable(${target} main.c)
  target_compile_definitions(${target} PRIVATE CUTILS_EMBTEST_SUITE=${FAET_SUITE_FN})
  target_link_libraries(${target} PRIVATE embtest_common)
  target_link_options(${target} PRIVATE ${embtest_link_flags})
  target_compile_features(${target} PRIVATE c_std_11)
  set_target_properties(${target} PROPERTIES FOLDER tests/freertos)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>)

  if(QEMU_SYSTEM_ARM)
    add_test(
      NAME ${target}
      COMMAND
        ${QEMU_SYSTEM_ARM} -machine mps2-an500 -cpu cortex-m7 -m 16M -kernel
        $<TARGET_FILE:${target}> -nographic -semihosting)
    set_tests_properties(${target} PROPERTIES TIMEOUT ${FAET_TIMEOUT})
  else()
    message(STATUS "qemu-system-arm not found; skipping ctest registration for ${target}")
  endif()
endfunction()

# --- opt-in aggregate runner (all suites in one QEMU boot) -------------------
# Strictly faster than N per-suite boots for a known-green full run, so kept as
# an opt-in convenience for local smoke tests. Default OFF: the per-suite
# embtest_<suite> ctests above are the CI granularity.
option(CUTILS_FREERTOS_AGGREGATE_RUNNER
       "Build the all-in-one embtest_runner ELF (one QEMU boot for all suites)" OFF)
if(CUTILS_FREERTOS_AGGREGATE_RUNNER)
  add_executable(embtest_runner main.c)
  target_link_libraries(embtest_runner PRIVATE embtest_common)
  target_link_options(embtest_runner PRIVATE ${embtest_link_flags})
  target_compile_features(embtest_runner PRIVATE c_std_11)
  set_target_properties(embtest_runner PROPERTIES FOLDER tests/freertos)

  add_custom_command(
    TARGET embtest_runner
    POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:embtest_runner>)

  if(QEMU_SYSTEM_ARM)
    add_test(
      NAME embtest_runner
      COMMAND
        ${QEMU_SYSTEM_ARM} -machine mps2-an500 -cpu cortex-m7 -m 16M -kernel
        $<TARGET_FILE:embtest_runner> -nographic -semihosting)
    # One boot runs everything, so it needs a longer budget than a single suite.
    set_tests_properties(embtest_runner PROPERTIES TIMEOUT 120)
  else()
    message(STATUS "qemu-system-arm not found; skipping ctest registration for embtest_runner")
  endif()
endif()

# --- per-suite runners -------------------------------------------------------
# Mirrors the suite list in main.c's aggregate branch (and the hosted
# all_embunit_tests.c), now surfaced as first-class ctests. The previously-
# orphaned dispatch_queue_timer_get_tests is excluded here — see note below.
freertos_add_embunit_test(NAME klist                 SUITE_FN klist_get_tests)
freertos_add_embunit_test(NAME mem_ring_buffer       SUITE_FN mem_ring_buffer_get_tests)
freertos_add_embunit_test(NAME os_mutex              SUITE_FN os_mutex_get_tests)
freertos_add_embunit_test(NAME os_event_flag         SUITE_FN os_event_flag_get_tests)
freertos_add_embunit_test(NAME os_task               SUITE_FN os_task_get_tests)
freertos_add_embunit_test(NAME queue_free_list       SUITE_FN queue_free_list_get_tests)
freertos_add_embunit_test(NAME queue_kqueue          SUITE_FN queue_kqueue_get_tests)
freertos_add_embunit_test(NAME queue_ts_queue_simple SUITE_FN queue_ts_queue_simple_get_tests)
freertos_add_embunit_test(NAME queue_ts_queue        SUITE_FN queue_ts_queue_get_tests)
freertos_add_embunit_test(NAME pool                  SUITE_FN pool_get_tests)
freertos_add_embunit_test(NAME notifier              SUITE_FN notifier_get_tests)
freertos_add_embunit_test(NAME notifier_static_store SUITE_FN notifier_static_store_get_tests)
freertos_add_embunit_test(NAME accumulator           SUITE_FN accumulator_get_tests)
# dispatch_queue_timer is intentionally excluded: dispatch_queue_timer_get_tests
# is #if 0'd out in tests/dispatch_queue_tests.c because it calls
# dispatch_stop_repeated_f, which is not yet implemented in
# src/dispatch_queue.c. Enabling the suite is feature work tracked
# separately, not part of this split.
freertos_add_embunit_test(NAME dispatch_queue        SUITE_FN dispatch_queue_get_tests)
freertos_add_embunit_test(NAME asyncio               SUITE_FN asyncio_get_tests)
freertos_add_embunit_test(NAME state_event_loop      SUITE_FN state_event_loop_get_tests)
