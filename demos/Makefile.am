# $Id: Makefile.am 2927 2009-09-08 15:26:45Z sloot $
# $URL: $

AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++0x

if WANT_OMP
noinst_PROGRAMS = sockettestServer sockettestClient
else
noinst_PROGRAMS =
endif

LDADD = ../src/libtimblserver.la

if WANT_OMP
sockettestClient_SOURCES = sockettestClient.cxx
sockettestServer_SOURCES = sockettestServer.cxx
endif

exdir = $(datadir)/doc/@PACKAGE@/examples
