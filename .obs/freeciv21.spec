#
# RPM spec file for freeciv21 to run on Open Build Service (OBS)
# Project Home: https://build.opensuse.org/project/show/home:longturn
#
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
# SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
#
Name:           freeciv21
Version:        3.2
Release:        %autorelease -p pre -s %{__scm_source_timestamp}
License:        GPL-3.0
Group:          Amusements/Games/Strategy/Turn Based
Summary:        Develop Your Civilization from Humble Roots to a Global Empire
Url:            https://github.com/longturn/freeciv21/
BuildRequires:  cmake
BuildRequires:  gcc gcc-c++
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
BuildRequires:  lua-devel gettext
BuildRequires:  ocl-icd
Recommends:     freeciv21-lang
Recommends:     alerque-libertinus-fonts
%endif

%if !0%{?fedora}
BuildRequires:  qt6-base-devel
BuildRequires:  qt6-svg-devel
BuildRequires:  qt6-multimedia-devel
BuildRequires:  kf6-karchive-devel
BuildRequires:  sqlite3-devel
BuildRequires:  lua53-devel
BuildRequires:  python313-sphinx
%endif

%if 0%{?suse_version}
Recommends:     freeciv21-lang
Recommends:     libertinus-fonts
%endif

BuildRoot:      %{_tmppath}/%{name}-%{__scm_source_timestamp}-build

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
Summary:    Translation files for freeciv21
Group:      Amusements/Games/Strategy/Turn Based
Requires:   freeciv21 = %{version}
BuildArch:  noarch

%description lang
Translation files for freeciv21.
%endif

%prep
%setup -q -n freeciv21-%{version}

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
%{_bindir}/freeciv21-*
%{_datadir}/freeciv21/
%{_datadir}/applications/net.longturn.freeciv21.desktop
%{_datadir}/applications/net.longturn.freeciv21.server.desktop
%{_datadir}/applications/net.longturn.freeciv21.modpack.desktop
%{_datadir}/applications/net.longturn.freeciv21.ruledit.desktop
%{_datadir}/metainfo/net.longturn.freeciv21.metainfo.xml
%{_datadir}/metainfo/net.longturn.freeciv21.server.metainfo.xml
%{_datadir}/metainfo/net.longturn.freeciv21.modpack.metainfo.xml
%{_datadir}/metainfo/net.longturn.freeciv21.ruledit.metainfo.xml
%{_mandir}/man6/freeciv21-*

%files lang -f %{name}-core.lang -f %{name}-nations.lang

%autochangelog
