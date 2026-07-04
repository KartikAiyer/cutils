# FreeRTOS.cmake -- pull FreeRTOS-Kernel via FetchContent and wire its CMake target.
#
# Usage (from the root CMakeLists.txt, only when CUTILS_PLATFORM_TYPE STREQUAL freertos):
# include("${CMAKE_SOURCE_DIR}/cmake/FreeRTOS.cmake")
#
# Upstream FreeRTOS-Kernel ships its own CMakeLists.txt that builds the `freertos_kernel` STATIC
# library. FetchContent_MakeAvailable() auto-calls add_subdirectory() on it, so we get that target
# for free. What upstream requires from us BEFORE that call: 1. a `freertos_config` INTERFACE target
# whose include dir holds FreeRTOSConfig.h 2. FREERTOS_PORT set to the port name (GCC_ARM_CM7 ->
# portable/GCC/ARM_CM7/r0p1) 3. FREERTOS_HEAP only if dynamic allocation is used (we omit it:
# static-only)
#
# References: - FetchContent_Declare / FetchContent_MakeAvailable: CMake docs (module FetchContent)
# - Upstream CMake contract: FreeRTOS-Kernel/CMakeLists.txt (top-level) + portable/CMakeLists.txt
# (port selection).

include(FetchContent)

# --- Pin a release. V11.3.0 commit 9b777ae5c5b8e9e456065a00294d1e5f5f9facf5 ---
# A tag is readable; a hash is tamper-evident. Override either via -D at configure time without
# editing this file:
if(NOT DEFINED FREERTOS_KERNEL_GIT_TAG)
  set(FREERTOS_KERNEL_GIT_TAG
      V11.3.0
      CACHE STRING "FreeRTOS-Kernel git ref to fetch")
endif()

FetchContent_Declare(
  freertos_kernel
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
  GIT_TAG ${FREERTOS_KERNEL_GIT_TAG}
  GIT_SHALLOW TRUE)

# --- Select the Cortex-M7 (r0p1) GCC port. Works for all M7 revisions per ---
# --- FreeRTOS's portable/GCC/ARM_CM7/ReadMe.txt; QEMU's cortex-m7 is r1p2. ---
if(NOT DEFINED CUTILS_FREERTOS_PORT)
  message(FATAL_ERROR "You have not defined a target chip for FreeRTOS")
else()
  set(FREERTOS_PORT
      ${CUTILS_FREERTOS_PORT}
      CACHE STRING "FreeRTOS port name (see portable/CMakeLists.txt)")
endif()

# --- No FREERTOS_HEAP: --- so configSUPPORT_DYNAMIC_ALLOCATION=0

# --- Provide the freertos_config INTERFACE target upstream requires.       ---
# FreeRTOSConfig.h lives with the FreeRTOS test harness (tests/freertos/) and is configured by that
# subdirectory. The caller must have placed the config directory in FREERTOS_CONFIG_INCLUDE_DIR
# before including this file.
if(NOT DEFINED FREERTOS_CONFIG_INCLUDE_DIR)
  message(FATAL_ERROR "FREERTOS_CONFIG_INCLUDE_DIR must be set to the directory containing "
                      "FreeRTOSConfig.h before including cmake/FreeRTOS.cmake")
endif()

if(NOT TARGET freertos_config)
  add_library(freertos_config INTERFACE)
  target_include_directories(freertos_config SYSTEM INTERFACE ${FREERTOS_CONFIG_INCLUDE_DIR})
endif()

# --- Populate + add_subdirectory upstream FreeRTOS-Kernel -> freertos_kernel ---
FetchContent_MakeAvailable(freertos_kernel)
