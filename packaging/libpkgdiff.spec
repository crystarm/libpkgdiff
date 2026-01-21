%define _unpackaged_files_terminate_build 1

Name:           libpkgdiff
Version:        1.1
Release:        alt1
Summary:        ALT Package Comparator (CLI with a bundled private library)
Group:          System/Utilities
License:        GPLv2
URL:            https://github.com/crystarm/libpkgdiff
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkg-config
BuildRequires:  libcurl-devel
BuildRequires:  libjansson-devel

# Optional but explicit; SONAME auto-reqs usually suffice
Requires:       libcurl
Requires:       libjansson4
Requires:       ca-certificates

%description
Library and CLI tool to compare ALT Linux package lists between branches p11 and sisyphus.

%prep
%setup -q

%build
%make_build

%install
rm -rf %{buildroot}
install -Dpm 0755 pkgdiff %{buildroot}%{_bindir}/pkgdiff
install -Dpm 0755 libpkgdiff.so %{buildroot}%{_libdir}/libpkgdiff.so

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%doc README* LICENSE* COPYING*
%{_bindir}/pkgdiff
%{_libdir}/libpkgdiff.so

%changelog
* Mon Nov 10 2025 Your Name <you@example.com> 1.1-alt1
- Initial ALT package: CLI + bundled private libpkgdiff.so
