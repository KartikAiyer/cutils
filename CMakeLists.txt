cmake_minimum_required(VERSION 3.14)
project(
  cutils
  VERSION 0.0.1
  LANGUAGES C
)

function(print_list theList)
  foreach(item ${theList})
    message("${item}")
  endforeach()
endfunction()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Configure the platform specific stuff Currently only C11 APIs supported
set(CUTILS_SUPPORTED_PLATFORM_TYPES pthread c11 freertos)

set(CUTILS_PLATFORM_TYPE
    pthread
    CACHE STRING "cutils OS abstraction platform (pthread|c11|freertos)"
)
set_property(CACHE CUTILS_PLATFORM_TYPE PROPERTY STRINGS pthread c11 freertos)

set(CUTILS_FREERTOS_PORT
    GCC_ARM_CM7
    CACHE STRING "FreeRTOS port identifier (see FreeRTOS portable/CMakeLists.txt)."
)

set(CUTILS_FREERTOS_SOURCE
    fetch
    CACHE STRING "freertos source: fetch (cutils FetchContent) | external (consumer provides)"
)
set_property(CACHE CUTILS_FREERTOS_SOURCE PROPERTY STRINGS fetch external)
set(CUTILS_SUPPORTED_FREERTOS_SOURCE fetch external)

# cmake-format: off
if(NOT CUTILS_PLATFORM_TYPE IN_LIST CUTILS_SUPPORTED_PLATFORM_TYPES)
  message(FATAL_ERROR "CUTILS_PLATFORM_TYPE must be 'pthread' or 'c11' or 'freertos, got '${CUTILS_PLATFORM_TYPE}'")
endif()

if(CUTILS_PLATFORM_TYPE STREQUAL freertos)
  if(NOT CUTILS_FREERTOS_SOURCE IN_LIST CUTILS_SUPPORTED_FREERTOS_SOURCE)
    message(FATAL_ERROR "CUTILS_FREERTOS_SOURCE must be 'fetch' or 'external', got '${CUTILS_FREERTOS_SOURCE}'")
  endif()
  if(CUTILS_FREERTOS_SOURCE STREQUAL fetch)
    set(CUTILS_FREERTOS_SUPPORTED_PORTS GCC_ARM_CM7)
    if(NOT CUTILS_FREERTOS_PORT IN_LIST CUTILS_FREERTOS_SUPPORTED_PORTS)
      message(STATUS "Supported PORTS in Self Test:")
      print_list(${CUTILS_FREERTOS_SUPPORTED_PORTS})
      message(FATAL_ERROR
              "CUTILS_FREERTOS_PORT='${CUTILS_FREERTOS_PORT}' is not supported by cutils self test")
    endif()
  else()
    if(NOT DEFINED FREERTOS_CONFIG_INCLUDE_DIR)
      message(FATAL_ERROR 
              "For an External Freertos build you must provide FREERTOS_CONFIG_INCLUDE_DIR"
              " that points to the folder which contains your FreeRTOSConfig.h")
    endif()
    if(NOT DEFINED CUTILS_FREERTOS_PORT)
      message(FATAL_ERROR "You must provide CUTILS_FREERTOS_PORT, see FreeRTOS portable/CMakeLists.txt")
    endif()
  endif()
endif()
# cmake-format: on

function(freertos_include)
  if(NOT DEFINED FREERTOS_CONFIG_INCLUDE_DIR)
    set(FREERTOS_CONFIG_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/tests/freertos/${CUTILS_FREERTOS_PORT}")
  endif()
  include(FreeRTOS)
endfunction()

find_package(Git QUIET)
if(GIT_FOUND
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules"
)
  # Update submodules as needed (only when this branch actually declares any). master has no submodules (the test
  # framework was moved to extern/embunit and is no longer fetched); other branches that still carry
  # extern/FreeRTOS-Kernel keep a .gitmodules and this step runs for them.
  option(GIT_SUBMODULE "Check submodules during build" ON)
  if(GIT_SUBMODULE)
    message(STATUS "Submodule update")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). "
                      "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles."
  )
endif()
# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here Let's ensure -std=c++xx instead
  # of -std=g++xx
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Let's nicely support folders in IDE's
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Testing only available if this is the main app Note this needs to be done in the main CMakeLists since it calls
  # enable_testing, which must be in the main CMakeLists.
  include(CTest)

  # Docs only available if this is the main app
  #[[
  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
  #]]
else()
  # Expect some build properties to be set
  if(NOT DEFINED CUTILS_PLATFORM_TYPE)
    message(FATAL_ERROR "You must define a Platform type in CUTILS_PLATFORM_TYPE")
  endif()
  # In the case this is part of another project which wants you to pull the freertos code base, this will do that.
  if(CUTILS_PLATFORM_TYPE STREQUAL freertos AND CUTILS_FREERTOS_SOURCE STREQUAL fetch)
    freertos_include()
  endif()
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/inc/cutils/Version.h")

set(API_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/inc" "${CMAKE_CURRENT_BINARY_DIR}/inc/")
add_subdirectory(src)

if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND BUILD_TESTING)
  # Tests
  option(PACKAGE_TESTS "Build the tests" ON)
  if(PACKAGE_TESTS)
    if(CUTILS_PLATFORM_TYPE STREQUAL freertos)
      freertos_include()
    endif()
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()
