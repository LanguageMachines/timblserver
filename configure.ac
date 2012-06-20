#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL$

AC_PREREQ(2.61)
AC_INIT([timblserver],[1.6],[timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi
if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

# Checks for programs.
AC_PROG_CXX( [g++] )
AC_PROG_RANLIB

# libtool stuff
# unfortunately, a lot of old libtools hang around.
# so LT_INIT and LT_PREREQ cannot be used for now
#LT_PREREQ([2.2.0])
#LT_INIT
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

# when running tests, use CXX
AC_LANG([C++])

ACX_PTHREAD([],[AC_MSG_ERROR([We need pthread support!])])

if test x"$acx_pthread_ok" = xyes; then
   LIBS="$PTHREAD_LIBS $LIBS" \
   CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
fi

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

# Checks for timbl library.
PKG_PROG_PKG_CONFIG
# inspired by feh-1.3.4/configure.ac.  Tnx Tom Gilbert and feh hackers.
# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
if test "x$PKG_CONFIG_PATH" = x; then
    export PKG_CONFIG_PATH=""
fi
AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>; 
               note that you can install timbl in a non-default directory with
               ./configure --prefix=<DIR> in the timbl installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([timbl], [timbl >= 6.4.2] )
CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
LIBS="$LIBS $timbl_LIBS"

AX_OPENMP(
  [AC_MSG_NOTICE( we have Open MP )
   CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS"],
  [AC_MSG_NOTICE( We don't seem to have Open MP )] )

AM_CONDITIONAL( WANT_OMP, test "$OPENMP_CXXFLAGS" != "" )

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
