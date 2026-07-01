# EmbUnitTest.cmake -- helper macros for embUnit-based C test suites
# This replaces TestUtil.cmake which was tied to googletest.

#[[
  package_add_embunit_test(NAME <name> FILES <src1> [src2 ...] [LIBS <lib> ...])

  Creates an embUnit test executable and registers it with CTest.
  Equivalent to the old `package_add_test` target, but without Google Test.
]]
macro(package_add_embunit_test)
  cmake_parse_arguments(PAE "" "NAME" "FILES;LIBS" ${ARGN})

  set(LIBS "")
  if(PAE_LIBS)
    list(APPEND LIBS ${PAE_LIBS})
  endif()

  add_executable(${PAE_NAME} ${PAE_FILES})

  set_target_properties(${PAE_NAME} PROPERTIES C_STANDARD 11)

  target_include_directories(${PAE_NAME}
    PRIVATE "${PROJECT_SOURCE_DIR}/extern/embunit"
    PRIVATE "${API_INCLUDE_DIR}")

  target_link_libraries(${PAE_NAME} cutils embunit ${LIBS})

  add_test(NAME ${PAE_NAME} COMMAND ${PAE_NAME})

  set_target_properties(${PAE_NAME} PROPERTIES FOLDER tests)
endmacro()
