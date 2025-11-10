Name:           libpkgdiff
Version:        0.1
Release:        1%{?dist}
Summary:        ALT Package Comparator (library + CLI)
License:        MIT
URL:            https://example.invalid/libpkgdiff
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make, pkgconfig(libcurl), pkgconfig(jansson)

%description
Library and CLI tool to compare ALT Linux package lists between branches.

%prep
%setup -q

%build
make

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
install -m 0755 pkgdiff %{buildroot}%{_bindir}/pkgdiff
install -m 0644 libpkgdiff.so %{buildroot}%{_libdir}/libpkgdiff.so

%files
%{_bindir}/pkgdiff
%{_libdir}/libpkgdiff.so

%changelog
* Mon Nov 10 2025 Your Name <you@example.com> - 0.1-1
- Initial package
