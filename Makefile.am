# $Id: Makefile.am 3523 2010-01-18 16:14:30Z sloot $
# $URL: https://ilk.uvt.nl/svn/trunk/sources/Timbl6/Makefile.am $

ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs init m4

EXTRA_DIST = bootstrap AUTHORS TODO NEWS timblserver.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timblserver.pc

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
