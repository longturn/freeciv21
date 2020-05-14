# Detect Qt5 headers and libraries and set flag variables

AC_DEFUN([FC_QT5],
[
  if test "x$fc_qt5_usable" = "x" ; then
    FC_QT5_CPPFLAGS="-DQT_DISABLE_DEPRECATED_BEFORE=0x050200"
    case $host_os in 
    darwin*) build_mac=yes;;
    *) build_mac=no;;
    esac
    FC_QT5_GENERIC
  fi
])
 

AC_DEFUN([FC_QT5_GENERIC],
[
  AC_LANG_PUSH([C++])

  AC_ARG_WITH([qmake],
    AS_HELP_STRING([--with-qmake], [path to qmake]),
    [AC_MSG_CHECKING([Qt5 qmake])
     if test -x "$withval" ; then
       QMAKE=$withval
       AC_MSG_RESULT([found])
     else
       QMAKE=no
       AC_MSG_RESULT([not found])
     fi],
    [AC_PATH_PROG(QMAKE, qmake, no, [$PATH])])

  if test "x$QMAKE" != "xno" ; then
    AC_MSG_CHECKING([Qt5 headers])

    qt5_install_headers=`$QMAKE -query QT_INSTALL_HEADERS`
    FC_QT5_COMPILETEST([$qt5_install_headers])
  fi

  if test "x$qt5_headers" = "xyes" ; then
    AC_MSG_RESULT([found])

    AC_MSG_CHECKING([Qt5 libraries])

    qt5_install_libs=`$QMAKE -query QT_INSTALL_LIBS`
    FC_QT5_LINKTEST([$qt5_install_libs])
  fi

  if test "x$qt5_libs" = "xyes" ; then
    AC_MSG_RESULT([found])
    AC_MSG_CHECKING([for Qt >= 5.2])
    FC_QT52_CHECK
  fi

  AC_LANG_POP([C++])
  if test "x$fc_qt52" = "xyes" ; then
    AC_MSG_RESULT([ok])

    AC_MSG_CHECKING([the Qt 5 moc command])
    MOCCMD="`$QMAKE -query QT_INSTALL_BINS`/moc"
    if test -x "$MOCCMD" ; then
      fc_qt5_usable=true
      AC_SUBST([MOCCMD])
      AC_MSG_RESULT([$MOCCMD])
    else
      AC_MSG_RESULT([not found])
      fc_qt5_usable=false
    fi
  else
    AC_MSG_RESULT([not found])
    fc_qt5_usable=false
  fi
])

dnl Test if Qt headers are found from given path
AC_DEFUN([FC_QT5_COMPILETEST],
[
  CPPFADD=" -I$1 -I$1/QtCore -I$1/QtGui -I$1/QtWidgets"

  if test "x$emscripten" = "xyes" || test "x$build_mac" = "xyes" ; then
    CXXFADD=" -std=c++11"
  else
    CXXFADD=" -fPIC"
  fi

  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="${CPPFLAGS}${CPPFADD}"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
    [qt5_headers=yes
     FC_QT5_CPPFLAGS="${FC_QT5_CPPFLAGS}${CPPFADD}"],
    [CXXFLAGS_SAVE="${CXXFLAGS}"
     CXXFLAGS="${CXXFLAGS}${CXXFADD}"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
      [qt5_headers=yes
       FC_QT5_CPPFLAGS="${FC_QT5_CPPFLAGS}${CPPFADD}"
       FC_QT5_CXXFLAGS="${FC_QT5_CXXFLAGS}${CXXFADD}"])
     CXXFLAGS="${CXXFLAGS_SAVE}"])

  CPPFLAGS="$CPPFLAGS_SAVE"
])

dnl Check if the included version of Qt is at least Qt5.2
dnl Output: fc_qt52=yes|no
AC_DEFUN([FC_QT52_CHECK],
[
  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $FC_QT5_CPPFLAGS"
  CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $FC_QT5_CXXFLAGS"
  LIBS_SAVE="$LIBS"
  LIBS="${LIBS}${LIBSADD}"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <QtCore>]],[[
      #if QT_VERSION < 0x050200
        fail
      #endif
    ]])],
    [fc_qt52=yes],
    [fc_qt52=no])
  LIBS="$LIBS_SAVE"
  CPPFLAGS="${CPPFLAGS_SAVE}"
  CXXFLAGS="${CXXFLAGS_SAVE}"
])


dnl Test Qt application linking with current flags
AC_DEFUN([FC_QT5_LINKTEST],
[
  LIBSADD=" -L$1 -lQt5Gui -lQt5Core -lQt5Widgets"

  if test "x$emscripten" = "xyes" ; then
    qt5_install_plugins=`$QMAKE -query QT_INSTALL_PLUGINS`
    LDFADD=" --bind -Wl,--no-check-features"
    LIBSADD="$LIBSADD -lqtlibpng -lqtharfbuzz -lqtpcre2 -L$qt5_install_plugins/platforms -lqwasm -L$qt5_install_plugins/imageformats -lqgif -lqico -lqjpeg -lQt5EventDispatcherSupport -lQt5ServiceSupport -lQt5ThemeSupport -lQt5FontDatabaseSupport -lqtfreetype -lQt5FbSupport -lQt5EglSupport -lQt5PlatformCompositorSupport -lQt5DeviceDiscoverySupport"
  elif test "x$build_mac" = "xyes" ; then
    qt5_install_plugins=`$QMAKE -query QT_INSTALL_PLUGINS`
    LIBSADD="$LIBSADD -lz -lQt5AccessibilitySupport -lQt5ClipboardSupport -lQt5GraphicsSupport -lQt5ThemeSupport -lQt5DBus -lQt5FontDatabaseSupport -lqtlibpng -lqtharfbuzz -lqtpcre2 -lqtfreetype -L$qt5_install_plugins/imageformats -lqgif -lqico -lqjpeg -L$qt5_install_plugins/styles -lqmacstyle -L$qt5_install_plugins/bearer -lqgenericbearer -L$qt5_install_plugins/platforms -lqcocoa -L$qt5_install_plugins/printsupport -lcocoaprintersupport -Wl,-framework,CoreText -Wl,-framework,Carbon -Wl,-framework,QuartzCore -Wl,-framework,CoreVideo -Wl,-framework,IOSurface -Wl,-framework,ImageIO -Wl,-framework,Metal -Wl,-framework,CoreGraphics -Wl,-framework,SystemConfiguration -Wl,-framework,GSS -Wl,-framework,DiskArbitration -Wl,-framework,IOKit -Wl,-framework,AppKit -Wl,-framework,Security -Wl,-framework,ApplicationServices -Wl,-framework,CoreServices -Wl,-framework,CoreFoundation -Wl,-framework,Foundation -Wl,-framework,OpenGL -Wl,-framework,AGL -lcups"
  else
    LDFADD=""
  fi

  CPPFLAGS_SAVE="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $FC_QT5_CPPFLAGS"
  CXXFLAGS_SAVE="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $FC_QT5_CXXFLAGS"
  LIBS_SAVE="$LIBS"
  LIBS="${LIBS}${LIBSADD}"
  LDFLAGS_SAVE="$LDFLAGS"
  LDFLAGS="${LDFLAGS}${LDFADD}"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <QApplication>]],
[[int a; QApplication app(a, 0);]])],
[qt5_libs=yes
 FC_QT5_LIBS="${FC_QT5_LIBS}${LIBSADD}"
 FC_QT5_LDFLAGS="${FC_QT5_LDFLAGS}${LDFADD}"])
 LIBS="$LIBS_SAVE"
 LDFLAGS="$LDFLAGS_SAVE"
 CPPFLAGS="${CPPFLAGS_SAVE}"
 CXXFLAGS="${CXXFLAGS_SAVE}"
])
