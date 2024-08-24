AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++17 -W -Wall -O3 -g

LDADD = libtimblserver.la

bin_PROGRAMS = timblclient timblserver

timblclient_SOURCES = TimblClient.cxx
timblserver_SOURCES = TimblServer.cxx

lib_LTLIBRARIES = libtimblserver.la
libtimblserver_la_LDFLAGS= -version-info 5:0:0

libtimblserver_la_SOURCES = ClientBase.cxx TimblThread.cxx TcpServer.cxx \
	HttpServer.cxx JsonServer.cxx
