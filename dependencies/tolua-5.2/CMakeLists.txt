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
endif()
