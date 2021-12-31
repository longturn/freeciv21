Feature Macros
**************

This is a complete list of all Freeciv21 feature macros used thorough the source code.

.. code-block:: rst

    #elif defined(ENABLE_NLS) && defined(HAVE_STRCASECOLL)
    #elif defined(ENABLE_NLS) && defined(HAVE_STRICOLL)
    #elif defined(FREECIV_HAVE_PTHREAD)
    #elif defined(FREECIV_HAVE_WINTHREADS)
    #ifdef FREECIV_HAVE_C11_THREADS
    #ifdef FREECIV_HAVE_LIBBZ2
    #ifdef FREECIV_HAVE_LIBINTL_H
    #ifdef FREECIV_HAVE_LIBLZMA
    #ifdef FREECIV_HAVE_LIBREADLINE
    #ifdef FREECIV_HAVE_THREAD_COND
    #ifdef FREECIV_HAVE_TINYCTHR
    #ifdef HAVE_AT_QUICK_EXIT
    #ifdef HAVE_BZLIB_H
    #ifdef HAVE_CONFIG_H
    #ifdef HAVE_DIRECT_H
    #ifdef HAVE_FCDB
    #ifdef HAVE_FCDB_MYSQL
    #ifdef HAVE_FCDB_ODBC
    #ifdef HAVE_FCDB_POSTGRES
    #ifdef HAVE_FCDB_SQLITE3
    #ifdef HAVE_GETPWUID
    #ifdef HAVE_ICONV
    #ifdef HAVE_LANGINFO_CODESET
    #ifdef HAVE_LIBCHARSET
    #ifdef HAVE_LZMA_H
    #ifdef HAVE_MAPIMG_MAGICKWAND
    #ifdef HAVE_PWD_H
    #ifdef HAVE_STRCASESTR
    #ifdef HAVE_STRING_H
    #ifdef HAVE_STRINGS_H
    #if defined(ENABLE_NLS) && defined(HAVE__STRICOLL)
    #if defined(FREECIV_HAVE_LIBBZ2) || defined(FREECIV_HAVE_LIBLZMA)
    #if defined(FREECIV_HAVE_LIBLZMA)
    #ifndef FREECIV_HAVE_LIBREADLINE
    #ifndef FREECIV_HAVE_THREAD_COND
    #ifndef HAVE_FCDB
    *       (see #ifndef FREECIV_HAVE_THREAD_COND) */


Extracted with:

.. code-block:: rst

    find ai client common server tools utility \( -name '*.cpp' -o -name '*.h' -o -name '*.in' \) -exec grep -E '#(el)?if.+HAVE_' '{}' \; | sort | uniq


And we define:

.. code-block:: rst

    #define FREECIV_DEBUG $<CONFIG:Debug>
    #cmakedefine BUG_URL "${BUG_URL}"
    #cmakedefine AI_MOD_STATIC_TEX
    #cmakedefine IS_DEVEL_VERSION
    #cmakedefine AUDIO_SDL
    #cmakedefine ALWAYS_ROOT
    #cmakedefine HAVE_AT_QUICK_EXIT
    #cmakedefine HAVE_BZLIB_H
    #cmakedefine HAVE_DIRECT_H
    #cmakedefine HAVE_DLFCN_H
    #cmakedefine HAVE_EXECINFO_H
    #cmakedefine HAVE_LIBCHARSET_H
    #cmakedefine HAVE_LZMA_H
    #cmakedefine HAVE_MEMORY_H
    #cmakedefine HAVE_PWD_H
    #cmakedefine HAVE_STRINGS_H
    #cmakedefine HAVE_STRING_H
    #cmakedefine HAVE_SYS_FILE_H
    #cmakedefine HAVE_SYS_SIGNAL_H
    #cmakedefine HAVE_SYS_STAT_H
    #cmakedefine HAVE_SYS_TERMIO_H
    #cmakedefine HAVE_SYS_UTSNAME_H
    #cmakedefine HAVE_SYS_WAIT_H
    #cmakedefine HAVE_TERMIOS_H
    #cmakedefine HAVE_LIBINTL_H
    #cmakedefine HAVE_ICONV
    #cmakedefine HAVE_BACKTRACE
    #cmakedefine HAVE_GETPWUID
    #cmakedefine HAVE_STRCASECOLL
    #cmakedefine HAVE_STRCASESTR
    #cmakedefine HAVE_STRICOLL
    #cmakedefine HAVE__SETJMP
    #cmakedefine HAVE__STRCOLL
    #cmakedefine HAVE__STRICOLL
    #cmakedefine ENABLE_NLS
    #cmakedefine CUSTOM_CACERT_PATH
    #cmakedefine FREECIV_HAVE_LIBBZ2
    #cmakedefine FREECIV_HAVE_LIBLZMA
    #cmakedefine FREECIV_STORAGE_DIR "${FREECIV_STORAGE_DIR}"
    #cmakedefine FREECIV_AI_MOD_LAST ${FREECIV_AI_MOD_LAST}
    #cmakedefine FREECIV_C11_STATIC_ASSERT
    #cmakedefine FREECIV_STATIC_STRLEN
    #cmakedefine FREECIV_CXX11_STATIC_ASSERT
    #cmakedefine FREECIV_WEB
    #cmakedefine FREECIV_TESTMATIC
    #cmakedefine FREECIV_META_URL "${FREECIV_META_URL}"
    #cmakedefine FREECIV_RELEASE_MONTH
    #cmakedefine FREECIV_HAVE_C11_THREADS
    #cmakedefine FREECIV_HAVE_PTHREAD
    #cmakedefine FREECIV_HAVE_WINTHREADS
    #cmakedefine FREECIV_HAVE_TINYCTHR
    #cmakedefine FREECIV_HAVE_THREAD_COND
    #cmakedefine FREECIV_ENABLE_NLS
    #cmakedefine FREECIV_HAVE_LIBINTL_H
    #cmakedefine FREECIV_HAVE_LIBREADLINE
    #cmakedefine FREECIV_MSWINDOWS 1


Extracted with:

.. code-block:: rst

    grep define utility/cmake_freeciv_config.h.in
    grep cmakedefine utility/cmake_fc_config.h.in
