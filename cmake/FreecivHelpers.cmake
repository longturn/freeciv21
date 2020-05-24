# Wapper arount tolua
function(tolua_generate)
  cmake_parse_arguments(
    ARG "" "INPUT;HEADER;SOURCE;PACKAGE_NAME" "" ${ARGN})
  # Determine the tolua target name
  if(TARGET ToLuaProgram::tolua)
    set(prog ToLuaProgram::tolua)
  elseif(TARGET tolua_program)
    set(prog tolua_program)
  else()
    set(prog tolua-NOTFOUND)
  endif()
  # Generate
  add_custom_command(
    OUTPUT
      ${ARG_SOURCE} ${ARG_HEADER}
    COMMAND
      ${prog}
      -n ${ARG_PACKAGE_NAME}
      -o ${CMAKE_CURRENT_BINARY_DIR}/${ARG_SOURCE}
      -H ${CMAKE_CURRENT_BINARY_DIR}/${ARG_HEADER}
      ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_INPUT}
    VERBATIM
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_INPUT}
  )
endfunction()
