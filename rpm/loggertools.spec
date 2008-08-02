Summary: tools for flight recorders
Name: loggertools
Version: 0.0.2
Release: 1
License: GPL
Group: Applications/Communications
BuildPrereq: gcc >= 4, boost-devel >= 1.33, tetex, tetex-latex
Source: http://max.kellermann.name/download/loggertools/0.x/%{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-root

%description
loggertools is a collection of tools and libraries which talk to GPS
flight loggers.

%prep
%setup

%build
make all documentation

%install
rm -rf $RPM_BUILD_ROOT
install -d -m 0755 $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT PREFIX=/usr

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/*
%{_mandir}/man1/*.1.gz
%{_datadir}/doc/loggertools/loggertools.pdf

%changelog
* Sat Aug  2 2008 Max Kellermann <mk@cm4all.com> 0.0.2-1
- new upstream version

* Tue Mar  5 2007 Max Kellermann <mk@cm4all.com> 0.0.1-1
- initial RPM package
