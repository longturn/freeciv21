add_library(
  tolua
  STATIC
  src/lib/tolua_event.c
  src/lib/tolua_is.c
  src/lib/tolua_map.c
  src/lib/tolua_push.c
  src/lib/tolua_to.c
)

target_include_directories(tolua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tolua PUBLIC lua)

target_link_libraries(tolua PUBLIC lua)

# Build the program if it wasn't found
if(NOT TARGET ToLuaProgram::tolua)
  add_executable(
    tolua_program
    src/bin/toluabind.c
    src/bin/tolua.c
  )

  target_link_libraries(tolua_program tolua)

  # On emscripten, we need to enable nodefs for tolua to be any useful
  # Otherwise it cannot open files
  if (EMSCRIPTEN)
    target_compile_options(tolua_program PRIVATE "-O3")
    target_link_options(tolua_program PRIVATE "SHELL:-s NODERAWFS=1")
  endif()
endif()
