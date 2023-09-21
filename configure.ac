AC_INIT([saiagent], [1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_PATH_TOOL(PKG_CONFIG, pkg-config)
PKG_CHECK_MODULES([CHECK], [check])

# prestera driver headers
AC_SUBST(PRESTERA_INC_DIR)
PRESTERA_INC_DIR="linux/prestera"

# libnl/genl
AC_SUBST(LIBNL_LIBS)
AC_SUBST(LIBNL_CFLAGS)
LIBNL_CFLAGS=`$PKG_CONFIG --cflags libnl-genl-3.0`
LIBNL_LIBS=`$PKG_CONFIG --libs libnl-genl-3.0`

# SAI
AC_ARG_VAR([LIBSAI_CPPFLAGS], [Preprocessor flags for libsai])
AC_ARG_VAR([LIBSAI_CFLAGS], [Compiler flags for libsai])
AC_ARG_VAR([LIBSAI_LDFLAGS], [Linker flags for libsai])
AC_ARG_VAR([LIBSAI_LIBS], [Libraries to link for libsai])

if test "x$LIBSAI_CFLAGS" = "x"; then
  LIBSAI_CFLAGS="-I/usr/include/sai"
fi
if test "x$LIBSAI_CPPFLAGS" = "x"; then
  LIBSAI_CPPFLAGS=$LIBSAI_CFLAGS
fi

SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$LIBSAI_CPPFLAGS $CPPFLAGS"
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$LIBSAI_CFLAGS $CFLAGS"
AC_CHECK_HEADER([sai.h])
CPPFLAGS="$SAVE_CPPFLAGS"
CFLAGS="$SAVE_CFLAGS"

if test "x$ac_cv_header_sai_h" = "xyes"; then
  SAVE_LIBS="$LIBS"
  LIBS="$LIBSAI_LIBS $LIBS"
  SAVE_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LIBSAI_LDFLAGS $LDFLAGS"
  SAVE_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$LIBSAI_CPPFLAGS $CPPFLAGS"
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$LIBSAI_CFLAGS $CFLAGS"
  AC_CHECK_LIB([sai], [sai_api_initialize])
  LIBS="$SAVE_LIBS"
  LDFLAGS="$SAVE_LDFLAGS"
  CPPFLAGS="$SAVE_CPPFLAGS"
  CFLAGS="$SAVE_CFLAGS"
fi

if test "x$ac_cv_lib_sai_sai_api_initialize" != "xyes" || test "x$ac_cv_header_sai_h" != "xyes"; then
  AC_MSG_ERROR([SAI headers/library not found])
fi

AC_SUBST(LIBSAI_CFLAGS)
AC_SUBST(LIBSAI_CPPFLAGS)
AC_SUBST(LIBSAI_LDFLAGS)
AC_SUBST(LIBSAI_LIBS)

AC_OUTPUT
