# $Id: timblserver.spec 5780 2010-08-19 20:06:07Z joostvb $
# $URL: https://ilk.uvt.nl/svn/trunk/sources/TimblServer/timblserver.spec $

Summary: Server extensions for TiMBL
Name: timblserver
Version: 1.1
Release: 1
License: GPL
Group: Applications/System
URL: http://ilk.uvt.nl/timbl
Packager: Joost van Baal <joostvb-timbl@ad1810.com>
Vendor: ILK, http://ilk.uvt.nl/

Source: http://ilk.uvt.nl/downloads/pub/software/timblserver-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

Requires: timbl
BuildRequires: gcc-c++, timbl

%description
TimblServer is a TiMBL wrapper; it adds server functionality to TiMBL.  It
allows TiMBL to run multiple experiments as a TCP server, optionally via HTTP.

The Tilburg Memory Based Learner, TiMBL, is a tool for Natural Language
Processing research, and for many other domains where classification tasks are
learned from examples.

TimblServer is a product of the ILK (Induction of Linguistic Knowledge)
research group of the Tilburg University and the CNTS research group of the
University of Antwerp.

If you do scientific research in NLP, TimblServer will likely be of use to you.

%prep
%setup

%build
%configure

%install
%{__rm} -rf %{buildroot}
%makeinstall
%{__rm} %{buildroot}%{_libdir}/libTimblServer.la

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog NEWS README TODO
%{_datadir}/doc/%{name}/*.pdf
%{_libdir}/libTimblServer*
%{_bindir}/Timbl*
%{_includedir}/%{name}/*.h
%{_libdir}/pkgconfig/%{name}.pc

%changelog
* Mon Jan 31 2011 Joost van Baal <joostvb-timbl@ad1810.com> - 1.1-1
- New upstream release
* Thu Aug 19 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 1.0.0-2
- Don't install libTimblServer.la: some builds fail on its presence.
* Thu Aug 19 2010 Joost van Baal <joostvb-timbl@ad1810.com> - 1.0.0-1
- Initial release.

