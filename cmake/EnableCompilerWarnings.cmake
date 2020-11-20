# Lists of warnings to enable
set(GCC_WARNINGS all no-sign-compare)
set(CLANG_WARNINGS ${GCC_WARNINGS})

# Candidates for -Werror
set(GCC_ERROR_WARNINGS return-type)
set(CLANG_ERROR_WARNINGS ${GCC_ERROR_WARNINGS})

# Prepend -W and -Werror=
list(TRANSFORM GCC_WARNINGS PREPEND -W)
list(TRANSFORM CLANG_WARNINGS PREPEND -W)

if (FREECIV_ENABLE_WERROR)
  # Only ever use -Werror in debug mode.
  set(MAYBE_WERROR $<IF:$<CONFIG:Debug>,-Werror=,-W>)
else()
  set(MAYBE_WERROR -W)
endif()
list(TRANSFORM GCC_ERROR_WARNINGS PREPEND ${MAYBE_WERROR})
list(TRANSFORM CLANG_ERROR_WARNINGS PREPEND ${MAYBE_WERROR})

# Join the two groups
list(APPEND GCC_WARNINGS ${GCC_ERROR_WARNINGS})
list(APPEND CLANG_WARNINGS ${CLANG_ERROR_WARNINGS})

# Enable them
add_compile_options(
  "$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${CLANG_WARNINGS}>"
  "$<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>")
