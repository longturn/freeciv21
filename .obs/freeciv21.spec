# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
# SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
#
# RPM spec file for freeciv21 to run on Open Build Service (OBS)
# Project Home: https://build.opensuse.org/project/show/home:longturn
#
License:        GPL-3.0
Name:           freeciv21
Version:        3.2
Release:        %autorelease -p pre -s %{__scm_source_timestamp}
Group:          Amusements/Games/Strategy/Turn Based
Summary:        We develop and play Freeciv21. Develop Your Civilization from Humble Roots to a Global Empire!
Url:            https://github.com/longturn/freeciv21/
Source:         %{name}-%{version}.tar.xz
BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  readline-devel
BuildRequires:  zlib-devel
BuildRequires:  fdupes
BuildRequires:  pkgconfig(SDL2_mixer)

%if 0%{?fedora}
BuildRequires:  sqlite-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  kf6-karchive-devel
BuildRequires:  python3-sphinx
BuildRequires:  lua-devel
BuildRequires:  gettext
BuildRequires:  ocl-icd
Recommends:     freeciv21-lang
Recommends:     alerque-libertinus-fonts
%endif

%if 0%{?suse_version}
BuildRequires:  qt6-base-devel
BuildRequires:  qt6-svg-devel
BuildRequires:  qt6-multimedia-devel
BuildRequires:  kf6-karchive-devel
BuildRequires:  sqlite3-devel
BuildRequires:  lua53-devel
BuildRequires:  python311-sphinx
Recommends:     freeciv21-lang
Recommends:     libertinus-fonts
%endif

BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
 Freeciv21 is a free open source turn-based empire-building 4x strategy game,
 in which each player becomes the leader of a civilization. You compete
 against several opponents to build cities and use them to support a military
 and an economy. Players strive to complete an empire that survives all
 encounters with its neighbors to emerge victorious. Play begins at the dawn
 of history in 4,000 BCE.
 .
 Freeciv21 takes its roots in the well-known FOSS game Freeciv and extends it
 for more fun, with a revived focus on competitive multiplayer environments.
 Players can choose from over 500 nations and can play against the computer
 or other people in an active online community.
 .
 The code is maintained by the team over at Longturn.net and is based on the
 Qt framework. The game supports both hex and square tiles and is easily
 modified to create custom rules.

%if !0%{?fedora}
%lang_package
%endif

%if 0%{?fedora}
%package    lang
Summary:    Translation files for Freeciv21
Group:      Amusements/Games/Strategy/Turn Based
Requires:   %{name} = %{version}
BuildArch:  noarch

%description lang
Translation files for freeciv21.
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Debug \
       -DFREECIV_DOWNLOAD_FONTS=OFF \
       -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name}
%cmake_build

%install
%cmake_install
%find_lang %{name}-core
%find_lang %{name}-nations
%fdupes %{buildroot}/%{_datadir}/

%files
%defattr(-,root,root,-)
%doc AUTHORS README.md
%exclude %{_docdir}/%{name}/licenses
%exclude %{_docdir}/%{name}/INSTALL
%exclude %{_docdir}/%{name}/COPYING
%license COPYING
%{_bindir}/%{name}-*
%{_datadir}/%{name}/
%{_datadir}/applications/net.longturn.%{name}.desktop
%{_datadir}/applications/net.longturn.%{name}.server.desktop
%{_datadir}/applications/net.longturn.%{name}.modpack.desktop
%{_datadir}/applications/net.longturn.%{name}.ruledit.desktop
%{_datadir}/metainfo/net.longturn.%{name}.metainfo.xml
%{_datadir}/metainfo/net.longturn.%{name}.server.metainfo.xml
%{_datadir}/metainfo/net.longturn.%{name}.modpack.metainfo.xml
%{_datadir}/metainfo/net.longturn.%{name}.ruledit.metainfo.xml
%{_mandir}/man6/%{name}-*

%files lang -f %{name}-core.lang -f %{name}-nations.lang

%changelog
