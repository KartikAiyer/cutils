# Aggregate embUnit test runner for the FreeRTOS/AN500 target.
#
# Builds `embtest_runner` — a bare-metal ELF that runs every cutils embUnit
# suite under FreeRTOS on the QEMU mps2-an500 (Cortex-M7) machine. A ctest
# entry launches QEMU with -semihosting so semihost_exit(0|1) becomes the
# process exit code that ctest reads as pass/fail.

file(GLOB test_sources ${PROJECT_SOURCE_DIR}/tests/*.c)
list(REMOVE_ITEM test_sources ${PROJECT_SOURCE_DIR}/tests/all_embunit_tests.c)
list(APPEND test_sources main.c retarget.c startup.c)

add_executable(embtest_runner ${test_sources})
target_link_libraries(embtest_runner PUBLIC cutils platform_abstraction freertos_kernel embunit)
target_include_directories(
  embtest_runner
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/extern/embunit ${API_INCLUDES}
)
target_link_options(embtest_runner PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostartfiles)
target_compile_definitions(embtest_runner PRIVATE AGGREGATE_RUNNER)

add_custom_command(
  TARGET embtest_runner
  POST_BUILD
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:embtest_runner>
)

# --- ctest: launch the runner in QEMU -----------------------------------------
# QEMU's process exit code is the value passed to semihost_exit() in
# retarget.c (0 = all tests passed, 1 = at least one failure), so ctest gets a
# first-class pass/fail signal with the UART output as human-readable detail.
# A generous timeout guards against a hung runner (e.g. a deadlock that
# prevents semihost_exit from ever firing).
find_program(QEMU_SYSTEM_ARM qemu-system-arm)
if(QEMU_SYSTEM_ARM)
  add_test(
    NAME embtest_runner
    COMMAND
      ${QEMU_SYSTEM_ARM} -machine mps2-an500 -cpu cortex-m7 -m 16M -kernel
      $<TARGET_FILE:embtest_runner> -nographic -semihosting)
  set_tests_properties(embtest_runner PROPERTIES TIMEOUT 120)
else()
  message(STATUS "qemu-system-arm not found; skipping ctest registration for embtest_runner")
endif()
