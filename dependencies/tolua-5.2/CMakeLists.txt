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

if(BUILD_TOLUA)
  add_executable(
    tolua_exe
    src/bin/toluabind.c
    src/bin/tolua.c
  )
  target_link_libraries(tolua_exe PRIVATE tolua)
else()
  add_executable(tolua_exe IMPORTED GLOBAL)
  set_target_properties(tolua_exe PROPERTIES IMPORTED_LOCATION ${TOLUA_EXECUTABLE})
endif()