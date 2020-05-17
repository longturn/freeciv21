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
target_link_libraries(tolua PUBLIC lua)