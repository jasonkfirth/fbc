Name:           freebasic
Version:        1.20.1
Release:        1%{?dist}
Summary:        FreeBASIC compiler

License:        GPL-2.0-or-later AND LGPL-2.1-or-later
URL:            https://www.freebasic.net/
Source0:        %{name}-%{version}.tar.xz

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconfig
BuildRequires:  rsync
BuildRequires:  dos2unix
BuildRequires:  ncurses-devel
BuildRequires:  gpm-devel
BuildRequires:  libffi-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  libX11-devel
BuildRequires:  libXext-devel
BuildRequires:  libXpm-devel
BuildRequires:  libXrandr-devel
BuildRequires:  libXrender-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  mesa-libGLU-devel

Requires:       gcc
Requires:       binutils
Requires:       ncurses-libs
Requires:       gpm-libs
Requires:       libffi
Requires:       alsa-lib
Requires:       pulseaudio-libs
Requires:       libX11
Requires:       libXext
Requires:       libXpm
Requires:       libXrandr
Requires:       libXrender
Requires:       mesa-libGL
Requires:       mesa-libGLU

%description
FreeBASIC is a free, open source BASIC compiler for modern platforms. It
includes the compiler, runtime libraries, graphics support, sound support,
headers, examples, and documentation needed to build FreeBASIC programs.

%prep
%autosetup -n %{name}-%{version}

%build
%make_build bootstrap-minimal
%make_build all FBC=bootstrap/fbc

%install
%make_install prefix=%{_prefix}

%files
%license copying.txt lgpl.txt
%doc readme.txt changelog.txt
%{_bindir}/fbc
%{_bindir}/fbcmkdep
%{_includedir}/freebasic
%{_libdir}/freebasic
%{_datadir}/doc/freebasic
%{_mandir}/man1/fbc.1*

%changelog
* Fri Apr 24 2026 FreeBASIC packagers <packagers@example.invalid> - 1.20.1-1
- Initial source package metadata.
