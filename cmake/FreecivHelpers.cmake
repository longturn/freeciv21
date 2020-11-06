include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Wapper around tolua
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

# Adds a PUBLIC flag to target if supported by the compiler
function(freeciv_add_flag_if_supported target flag)
  string(MAKE_C_IDENTIFIER "${flag}" varname)
  string(TOUPPER "${flag}" varname)
  check_c_compiler_flag("${flag}" "FREECIV_C_COMPILER_HAS_${varname}")
  check_cxx_compiler_flag("${flag}" "FREECIV_CXX_COMPILER_HAS_${varname}")
  if (FREECIV_C_COMPILER_HAS_${varname}
      AND FREECIV_CXX_COMPILER_HAS_${varname})
    target_compile_options(${target} PUBLIC "${flag}")
  endif()
endfunction()

function(update_default_translations target)
  if(FREECIV_ENABLE_NLS AND NOT TRANSLATIONS_GENERATED)
    add_dependencies(${target} freeciv-core.pot-update)
    add_dependencies(${target} freeciv-nations.pot-update)
    add_dependencies(${target} update-po)
    add_dependencies(${target} update-gmo)
  endif()
  set(TRANSLATIONS_GENERATED TRUE)
endfunction()
