# C Utils

Useful abstraction built using static allocation. This is useful to create an event processing systems, state machines, async io and publisher/subscriber interfaces using static allocation. Good for an RTOS environment.


## Current flavors
The abstractions include a OS abstraction layer. This repository currently provides a pthread implementation for these os primitives. The remainging abstraction are built around these abstractions. The default cmake options will select the pthread os implementation. There is an implementaion based on a c11 implementation which eventually calls into pthread. This will be phased out. Use the CMAKE gui to find the optoin flags that control this. If you port this for your platform, you will need to provide the os abstraction layer and update the cmake build to allow a user to select your platform as an option. 


## Build
If you have a system with c11 compliant compiler, cmake and pthreads, you can build the code base easily. The main deliverable is the cutils library which can be used to link against your application code. The other target are the various unit tests for the different utilities provided. 

### Checkout
```
git clone https://github.com/KartikAiyer/cutils
git submodule update --init --recursive
mkdir build
cd build 
cmake -G "Your generator of choice" ..
```

You can then build the individual targets based on your generator.

### Presets (recommended)

`CMakePresets.json` defines first-class configure/build/test presets for each
platform flavor with Debug/Release splits, and always emits
`compile_commands.json` (for clangd/IDE indexing):

| preset              | platform | notes                                                                           |
| ------------------- | -------- | ------------------------------------------------------------------------------ |
| `pthread-debug`     | pthread  | host (Linux/macOS), runs under ctest                                          |
| `pthread-release`   | pthread  | host, optimized                                                              |
| `pthread-rt-debug`  | pthread  | real-time (`SCHED_RR`); needs `CAP_SYS_NICE` — production embedded-Linux target |
| `c11-debug`         | c11      | host C11 threads (calls into pthread)                                        |
| `c11-release`       | c11      | host, optimized                                                             |
| `freertos-debug`    | freertos | M7 QEMU (`mps2-an500`) cross-build; needs arm-none-eabi toolchain + `qemu-system-arm` |
| `freertos-release`  | freertos | M7 QEMU, optimized                                                          |

```
# configure + build + test the host pthread flavor
cmake --preset pthread-debug
cmake --build --preset pthread-debug
ctest --preset pthread-debug

# FreeRTOS/M7 QEMU target (cross toolchain + qemu-system-arm on PATH)
cmake --preset freertos-debug
cmake --build --preset freertos-debug
ctest --preset freertos-debug
```

Build dirs land under `build/<preset-name>/`. `CMAKE_EXPORT_COMPILE_COMMANDS`
is set in `CMakeLists.txt`, so `compile_commands.json` is generated in each
build dir for Make/Ninja generators (it is gitignored). Keep personal overrides
in `CMakeUserPresets.json` (gitignored, not committed).

### Scheduling policy & task priorities

The pthread/c11 ports select their POSIX scheduling policy via the
`CUTILS_PTHREAD_SCHED_POLICY` cache variable (default `SCHED_OTHER`). This
single knob drives both the `CUTILS_TASK_PRIORITY_*` range and whether
`task_new_static()` takes explicit control of thread scheduling:

| policy        | priority range | `.priority` honored? | requires privilege?            |
| ------------- | -------------- | ------------------- | ------------------------------ |
| `SCHED_OTHER` | 0..0           | no (no-op)          | no — host/CI default           |
| `SCHED_FIFO`  | 1..99          | yes                 | yes (`CAP_SYS_NICE` on Linux)  |
| `SCHED_RR`    | 1..99          | yes                 | yes (`CAP_SYS_NICE` on Linux)  |

- Under `SCHED_OTHER` (the default, used by `pthread-debug`/`c11-debug`),
  `sched_get_priority_{min,max}` are both `0`, so every `CUTILS_TASK_PRIORITY_*`
  collapses to `0` and `task_create_params_t::priority` is silently ignored.
  `task_new_static()` uses `PTHREAD_INHERIT_SCHED`, so the build runs
  unprivileged. The abstraction here is a host convenience, not a real
  scheduler — this is what keeps the host `ctest` suites green without root.
- Under `SCHED_FIFO`/`SCHED_RR` (e.g. the `pthread-rt-debug` preset),
  priorities are real and `.priority` is applied via `PTHREAD_EXPLICIT_SCHED`,
  which on Linux requires `CAP_SYS_NICE`. Intended for the embedded-Linux
  (e.g. PREEMPT_RT) production target; `pthread_create()` returns `EPERM`
  without the capability.
- The FreeRTOS port is where priorities are fully real with no privilege
  requirement.

Override at configure time, e.g.:

```
cmake --preset pthread-debug -DCUTILS_PTHREAD_SCHED_POLICY=SCHED_FIFO
```

### Docs

Look throught the [wiki](https://github.com/KartikAiyer/cutils/wiki) to learn more about how to use the utilities.
