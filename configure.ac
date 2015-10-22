#                                              -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

# required versions of other packages
m4_define([glib_required_version], [2.36.0])
m4_define([libsoup_required_version], [2.26.0])
m4_define([gstreamer_required_version], [1.0.0])


AC_INIT([stream-decoder], [0.9])

AC_CONFIG_SRCDIR([src/StreamDecoder.cpp])

AM_INIT_AUTOMAKE(no-define dist-bzip2)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX

AC_MSG_CHECKING([whether to turn on debugging])
AC_ARG_ENABLE(debug,
              [  --enable-debug          turn on debugging (default=no)],
              if eval "test x$enable_debug = xyes"; then
                DEBUGFLAGS="-g -O0"
              fi)

if test -n "$DEBUGFLAGS"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

CXXFLAGS="$CXXFLAGS -Wall -std=c++0x -O3 $DEBUGFLAGS"

AM_PATH_GLIB_2_0(glib_required_version,,, gobject gthread)

PKG_CHECK_EXISTS(libsoup-2.4 >= libsoup_required_version)
PKG_CHECK_EXISTS(gstreamer-1.0 >= gstreamer_required_version)

PKG_CHECK_MODULES(STREAM_DECODER,
	libsoup-2.4 gstreamer-1.0 gobject-2.0 gthread-2.0 gio-unix-2.0)

AC_ARG_ENABLE([32bit],
  AS_HELP_STRING([--enable-32bit],
    [stream-decoders output will be 32bit per sample frame instead of 16])
)

AS_IF([test "x$enable_32bit" = "xyes"], [
  BITS_PER_SAMPLE=32
  CXXFLAGS="$CXXFLAGS -DUSE32BIT"
], [
  BITS_PER_SAMPLE=16
])

AC_PATH_TOOL(GDBUS_CODEGEN, gdbus-codegen)

AC_CONFIG_FILES([
Makefile
src/Makefile
tests/Makefile
stream-decoder.service
])

AC_OUTPUT

AC_MSG_RESULT([
stream-decoder configured for $BITS_PER_SAMPLE bits per sample output
])
