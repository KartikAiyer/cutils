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

### Docs

Look throught the [wiki](https://github.com/KartikAiyer/cutils/wiki) to learn more about how to use the utilities.
