# arm-none-eabi.cmake -- bare-metal ARM Cortex-M7 cross-build toolchain file.
#
# Invoke the FreeRTOS/M7 build with:
#   cmake -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake \
#         -DCUTILS_PLATFORM_TYPE=freertos \
#         -DBUILD_TESTING=ON \
#         <source-dir>
#
# This file is the contract between the user's installed cross toolchain and
# the cutils build. The repo does NOT vendor or install a compiler; it assumes
# a bare-metal ARM cross toolchain is already on PATH (providing
# arm-none-eabi-gcc / -ld / -objcopy / -size + newlib). See
# docs/freertos_port_plan.md §3a for install pointers.
#
# Target: QEMU `mps2-an500` (Cortex-M7, DP FPv5-D16). FPU flags verified
# against QEMU's MVFR0/MVFR1 in target/arm/tcg/cpu-v7m.c.

set(CMAKE_SYSTEM_NAME Generic)        # bare-metal (no OS)
set(CMAKE_SYSTEM_PROCESSOR cortex-m7)

# --- Compilers / binutils (looked up on PATH; override via -D if needed) ---
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++ CACHE FILEPATH "")
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE FILEPATH "")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE FILEPATH "")
set(CMAKE_SIZE arm-none-eabi-size CACHE FILEPATH "")
set(CMAKE_AR arm-none-eabi-ar CACHE FILEPATH "")
set(CMAKE_RANLIB arm-none-eabi-ranlib CACHE FILEPATH "")

# --- Don't attempt a hosted link test during configure (we are freestanding) ---
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- Target MCU flags: Cortex-M7, Thumb, DP FPv5-D16, hard-float ABI ---
# -mfpu=fpv5-d16 (NOT fpv5-sp-d16): QEMU's M7 has a double-precision unit, so
# both float and double are executed by the FPU. The sp-only variant would
# route double through software emulation despite the hardware DP unit.
# Declared as a CMake list (unquoted) so add_compile_options / add_link_options
# expand it as separate args, not one big quoted string. CMake lists are
# semicolon-separated internally; a quoted "-mcpu=... -mthumb ..." would be a
# single argument and gcc would reject it as one unknown -mcpu value.
set(MCU_FLAGS -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)

add_compile_options(
  ${MCU_FLAGS}
  -ffunction-sections
  -fdata-sections
  -ffreestanding
  --specs=nano.specs)

# --- Link flags ---
# --gc-sections drops unreferenced functions/data (e.g. unused FreeRTOS
# sources like croutine.c/stream_buffer.c pulled in by the upstream target).
# A link map is emitted next to the binary for size debugging.
# NOTE: rdimon.specs is intentionally NOT added -- semihosting is used only
# for SYS_EXIT via a small bkpt 0xAB helper in retarget.c; all I/O goes over
# the CMSDK UART0 at 0x40004000.
add_link_options(
  ${MCU_FLAGS}
  -Wl,--gc-sections
  -Wl,-Map=${CMAKE_PROJECT_NAME}.map
  --specs=nano.specs)

# --- Search defaults: never look in the host sysroot ---
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
