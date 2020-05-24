# Finds the tolua program

include(FindPackageHandleStandardArgs)

find_program(TOLUA_PROGRAM tolua)

find_package_handle_standard_args(
  ToLuaProgram DEFAULT_MSG TOLUA_PROGRAM)

if(ToLuaProgram_FOUND AND NOT TARGET ToLuaProgram::tolua)
  add_executable(ToLuaProgram::tolua IMPORTED)
  set_target_properties(ToLuaProgram::tolua PROPERTIES IMPORTED_LOCATION ${TOLUA_PROGRAM})
endif()
