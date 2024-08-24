#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([timblserver],[1.19],[lamasoftware@science.ru.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AX_REQUIRE_DEFINED([AX_CXX_COMPILE_STDCXX_17])

# Checks for programs.
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_17

# libtool stuff
LT_INIT


# when running tests, use CXX
AC_LANG([C++])

AX_PTHREAD([],[AC_MSG_ERROR([We need pthread support!])])

if test x"$ax_pthread_ok" = xyes; then
   LIBS="$PTHREAD_LIBS $LIBS" \
   CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
fi

AC_OPENMP
AM_CONDITIONAL( WANT_OMP, [test "x$ac_cv_prog_cxx_openmp" != "xunsupported"] )

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h sys/socket.h unistd.h ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SSIZE_T
AC_TYPE_SIZE_T
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([getaddrinfo gethostbyaddr gethostbyname inet_ntoa memset socket strerror dup2 memmove])

AX_LIB_SOCKET_NSL
AX_TYPE_SOCKLEN_T

PKG_PROG_PKG_CONFIG

if test "x$prefix" = "xNONE"; then
   prefix="/usr/local"
fi

if test "x$PKG_CONFIG_PATH" = x; then
    export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
else
    export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

AC_OSX_PKG( [icu4c] )
PKG_CHECK_MODULES([ticcutils], [ticcutils >= 0.28] )
CXXFLAGS="$CXXFLAGS $ticcutils_CFLAGS"
LIBS="$LIBS $ticcutils_LIBS"

PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.6.16] )
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$XML2_LIBS $LIBS"

PKG_CHECK_MODULES([timbl], [timbl >= 6.8] )
CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
LIBS="$LIBS $timbl_LIBS"

PKG_CHECK_MODULES([ICU], [icu-uc >= 50 icu-io] )
CXXFLAGS="$CXXFLAGS $ICU_CFLAGS"
LIBS="$ICU_LIBS $LIBS"

AC_CONFIG_FILES([
  Makefile
  timblserver.pc
  m4/Makefile
  src/Makefile
  docs/Makefile
  include/Makefile
  include/timblserver/Makefile
  demos/Makefile
])
AC_OUTPUT
