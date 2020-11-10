# backward-cpp configuration (used to print stack traces)
#
# We check for stack-unwinding libraries ourselves because the backward-cpp
# CMake configuration file triggers warnings.

include(CheckSymbolExists)

# Stack-unwinding library:
#   - native API on MSVC (MSYS and MINGW aren't ABI-compatible with it)
#   - libunwind if available
#   - backtrace() if everything else fails.
if (MSVC)
  message(STATUS "Using the Windows native API for stack unwinding. "
                 "This is the preferred option.")
  set(CAN_UNWIND_STACK TRUE)
endif()

# libunwind
if (NOT CAN_UNWIND_STACK)
  find_package(PkgConfig)
  if (PKG_CONFIG_FOUND)
    pkg_search_module(unwind IMPORTED_TARGET libunwind)
    if (unwind_FOUND)
      message(STATUS "Using libunwind for stack unwinding. "
                     "This is the preferred option.")
      set(CAN_UNWIND_STACK TRUE)
      set(STACK_UNWINDING_LIBRARY PkgConfig::unwind)
      set(BACKWARD_HAS_UNWIND 1)
    endif()
  endif()
endif()

# Last-resort: backtrace()
if (NOT CAN_UNWIND_STACK AND (NOT HAVE_EXECINFO_H OR NOT HAVE_BACKTRACE))
  check_include_file(execinfo.h HAVE_EXECINFO_H)
  check_symbol_exists(backtrace execinfo.h HAVE_BACKTRACE)
  if (NOT CAN_UNWIND_STACK AND HAVE_EXECINFO_H AND HAVE_BACKTRACE)
    message(STATUS "Using backtrace() for stack unwinding. "
                   "Install libunwind and pkg-config for better results.")
    set(CAN_UNWIND_STACK TRUE)
    set(BACKWARD_HAS_BACKTRACE 1)
  endif()
endif()

# Nothing found.
if (NOT CAN_UNWIND_STACK)
  message(WARNING "Could not find any supported stack unwinding library.")
endif()

# Debug information library. backtrace-cpp supports many options here, but only
# libdw comes with a pkg-config file. We additionally check for
# backtrace_symbols from libgcc.
# Options supported by backtrace-cpp: libdw, libdwarf, libbfd, backtrace_symbols
# Options supported by us: libdw, backtrace_symbol
if (CAN_UNWIND_STACK) # If we can't unwind, everything below is useless
  # Native Windows API
  if (MSVC)
    message(STATUS "Using the Windows native API to retrieve stack symbols. "
                   "This is the preferred option.")
    set(CAN_RETRIEVE_SYMBOLS TRUE)
  endif()

  # libdw
  if (NOT CAN_RETRIEVE_SYMBOLS)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_search_module(dw IMPORTED_TARGET libdw)
      if (dw_FOUND)
        message(STATUS "Using libdw to retrieve stack symbols. "
                       "This is the preferred option.")
        set(CAN_RETRIEVE_SYMBOLS TRUE)
        set(STACK_SYMBOLS_LIBRARY PkgConfig::dw)
        set(BACKWARD_HAS_DW 1)
      endif()
    endif()
  endif()

  # Last-resort: backtrace_symbol
  if (NOT CAN_RETRIEVE_SYMBOLS)
    find_package(PkgConfig)
    check_include_file(execinfo.h HAVE_EXECINFO_H)
    check_symbol_exists(backtrace_symbols execinfo.h HAVE_BACKTRACE_SYMBOLS)
    if (HAVE_EXECINFO_H AND HAVE_BACKTRACE_SYMBOLS)
      message(STATUS "Using backtrace_symbols() to retrieve stack symbols. "
                     "Install libdw and pkg-config for better results.")
      set(CAN_RETRIEVE_SYMBOLS TRUE)
      set(BACKWARD_HAS_BACKTRACE_SYMBOLS 1)
    endif()
  endif()

  # Nothing found.
  if (NOT CAN_RETRIEVE_SYMBOLS)
    message(WARNING "Could not find any supported stack unwinding library.")
  endif()
endif()
