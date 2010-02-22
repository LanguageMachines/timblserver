# $Id: Makefile.am 3499 2010-01-12 11:56:37Z sloot $
# $URL: $

AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = TimblClient TimblServer

TimblClient_SOURCES = TimblClient.cxx
TimblServer_SOURCES = TimblServer.cxx

lib_LTLIBRARIES = libTimblServer.la
libTimblServer_la_LDFLAGS= -version-info 2:0:0

libTimblServer_la_SOURCES = 
	SocketBasics.cxx ServerBase.cxx	TimblServerAPI.cxx
