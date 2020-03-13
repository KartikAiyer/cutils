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

### Docs

Look throught the wiki to learn more about how to use the utilities.
