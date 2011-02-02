# $Id$
# $URL$

ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs m4

EXTRA_DIST = bootstrap AUTHORS TODO NEWS timblserver.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timblserver.pc

ChangeLog: NEWS
	svn update; svn2cl --include-rev
