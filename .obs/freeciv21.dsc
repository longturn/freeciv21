# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
# SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
#
# DEB source cotrol file for freeciv21 to run on Open Build Service (OBS)
# Project Home: https://build.opensuse.org/project/show/home:longturn
#
Format: 1.0
Source: Freeciv21
Version: 3.2
Binary: freeciv21
Maintainer:
  Louis Moureaux <m_louis30@yahoo.com>,
  James Robertson <jwrober@gmail.com>
Architecture: any
Build-Depends:
  debhelper (>= 9),
  cmake,
  ninja-build,
  python3,
  python3-sphinx,
  gettext,
  libreadline-dev,
  qt6-base-dev,
  qt6-multimedia-dev,
  qt6-svg-dev,
  libkf6archive-dev,
  liblua5.3-dev,
  libsqlite3-dev,
  libsdl2-2.0-0 (>= 2.0.20),
  libsdl2-dev (>= 2.0.20),
  libsdl2-mixer-dev,
  libjack-dev,
  libavformat61 (>= 7:7.0),
  libavcodec61 (>= 7:7.0)
