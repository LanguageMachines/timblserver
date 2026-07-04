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
