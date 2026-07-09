add_executable(an500_smoke main.c retarget.c startup.c)
target_link_libraries(an500_smoke PUBLIC cutils platform_abstraction freertos_kernel embunit)
target_include_directories(an500_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_options(an500_smoke PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostartfiles)

add_custom_command(
  TARGET an500_smoke
  POST_BUILD
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:an500_smoke>
)
