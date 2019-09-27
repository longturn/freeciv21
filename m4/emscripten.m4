AC_DEFUN([FC_EMSCRIPTEN],
[
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE(
      [[#ifndef __EMSCRIPTEN__
         error fail
        #endif
      ]])],
      [emscripten=yes],
      [emscripten=no])
])
