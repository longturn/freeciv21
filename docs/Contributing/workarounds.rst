.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. include:: /global-include.rst

Workarounds
***********

This page lists workarounds in the code. They should be checked from time to
time and removed once they are no longer needed.

#. The file ``cmake/FindSDL2_mixer.cmake`` is needed for SDL2_mixer 2.0, which
   didn't have CMake support. CMake support was introduced in SDL2_mixer 2.5,
   but Ubuntu 22.04 is stuck with SDL2_mixer 2.0.4.
#. The macOS CI installs a fixed Python version to work around a bug in Meson.
   (``.github/workflows/build.yaml``)
