Name:     ydotool
Version:  0.1.9
Release:  0.1.20200218git.9c3a4e7%{?dist}.wef
Summary:  Generic command-line automation tool (no X!)
License:  MIT
URL:      https://github.com/ReimuNotMoe/ydotool
Source0:  https://github.com/ReimuNotMoe/ydotool/archive/%{name}-%{version}.tar.gz

# create patch with:
# diff -rNu -x build ydotool--orig ydotool-20200211
# removed 'static' build from CMakeList.txt (but libydotool.a still gets built)
# Patch0:   ydotool-patch0-remove-static

%global __strip /bin/strip

BuildRequires: boost-devel
BuildRequires: cmake
BuildRequires: libuInputPlus
BuildRequires: libevdevPlus
BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: systemd-rpm-macros
BuildRequires: scdoc

%description

Performs some of the functions of xdotool(1) without requiring X11 -
however, it generally requires root permission (to open /dev/uinput)

Currently implemented command(s):

    type - Type a string
    key - Press keys
    mousemove - Move mouse pointer to absolute position
    mousemove_relative - Move mouse pointer to relative position
    mouseup - Generate mouse up event
    mousedown - Generate mouse down event
    click - Click on mouse buttons
    recorder - Record/replay input events

N.B. optionally, you can start the ydotoold daemon with:

    systemctl enable ydotoold
    systemctl start ydotoold

%prep
%autosetup

%build
%cmake

%install
%make_install
mkdir -p %{buildroot}/%{_unitdir}
install -p Daemon/%{name}.service %{buildroot}/%{_unitdir}
# the macros _should_ take care of this, but no ...
mkdir -p %{buildroot}/%{_mandir}/man1
mkdir -p %{buildroot}/%{_mandir}/man8
scdoc < manpage/%{name}.1.scd | gzip -c > %{buildroot}/%{_mandir}/man1/%{name}.1.gz
scdoc < manpage/%{name}d.8.scd | gzip -c > %{buildroot}/%{_mandir}/man8/%{name}d.8.gz

%files
# unwanted huge static library:
%exclude %{_libdir}/libydotool.a
%{_libdir}/*
%{_unitdir}/*
%{_bindir}/%{name}*

%doc README.md
%{_mandir}/man1/%{name}.1.*
%{_mandir}/man8/%{name}d.8.*

%license LICENSE

%changelog
* Tue Feb 18 2020 Bob Hepple <bob.hepple@gmail.com> - 0.1.9-0.1.20200218git.9c3a4e7.fc31.wef
- rebuild from head to pick up manuals & service file
- remove static build
- strip binaries (rpmlint complained about them)
* Mon Feb 17 2020 Bob Hepple <bob.hepple@gmail.com> - 0.1.8.20200211.3.fc31.wef
- add BuildRequires: systemd-rpm-macros; add %%dist to release
* Sun Feb 16 2020 Bob Hepple <bob.hepple@gmail.com> - 0.1.8.20200211.2.wef
- use %%_unitdir
* Sun Feb 16 2020 Bob Hepple <bob.hepple@gmail.com> - 0.1.8.20200211.1.wef
- Initial version of the package
