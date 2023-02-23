..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 zekoz

Setting up MSYS2 for Windows
****************************

This document is about building and compiling Freeciv21 with MSYS2 on Windows. MSYS2 is available from
https://www.msys2.org. The Longturn Community builds the Windows Installer packages using MSYS2. If you
are looking for a first class development environment on Windows, have a look at :doc:`visual-studio`.


Setup
=====

This chapter is about creating a new MSYS2 build environment.

#. Download MSYS2 from their homepage: https://www.msys2.org/#installation

#. Run the downloaded file to install MSYS2 on you build system. This author recommends that you install the
   files in a directory off of the root of your :file:`C:\\` Drive. For example: :file:`C:\\Tools`

#. Launch :file:`msys2_shell` from the installation directory. Issue the following command to update the base
   set of packages.

    .. code-block:: sh

        > pacman -Syuu


#. The :file:`msys2_shell` will close when this is complete. Run it again and install following packages with
   :code:`pacman -Su <package name>`. The packages needed for building Freeciv21 are numerous. These packages
   are needed even if you do not plan to make the installer, update documentation, but only build Freeciv21
   for local use.

#. Install these arch independent packages that are needed for building Freeciv21. With these packages it is
   possible to build a Freeciv21 source tree that was created via release tarball or zip file.

    * pkgconf
    * tar
    * git
    * automake
    * libtool
    * autoconf
    * patch
    * ed
    * python3
    * python-pip
    * make

#. Install these arch-specific packages for building common parts (i686 or x86_64, but not both). It is
   recommended that you install the x86_64 packages, unless you think you need a cross-compiler.

    * mingw-w64-i686-gcc / mingw-w64-x86_64-gcc
    * mingw-w64-i686-gdb / mingw-w64-x86_64-gdb
    * mingw-w64-i686-curl / mingw-w64-x86_64-curl
    * mingw-w64-i686-bzip2 / mingw-w64-x86_64-bzip2
    * mingw-w64-i686-readline / mingw-w64-x86_64-readline
    * mingw-w64-i686-lua / mingw-w64-x86_64-lua
    * mingw-w64-i686-imagemagick / mingw-w64-x86_64-imagemagick
    * mingw-w64-i686-SDL2_mixer / mingw-w64-x86_64-SDL2_mixer
    * mingw-w64-i686-cmake / mingw-w64-x86_64-cmake
    * mingw-w64-i686-ninga / mingw-w64-x86_64-ninja
    * mingw-w64-i686-libunwind / mingw-w64-x86_64-libunwind
    * mingw-w64-i686-make / mingw-w64-x86_64-make
    * mingw-w64-i686-python3 / mingw-w64-x86_64-python3
    * mingw-w64-i686-python-pip / mingw-w64-x86_64-python-pip

#. Arch-specific optional packages for building common parts (i686 or x86_64, but not both).

    * mingw-w64-i686-drmingw / mingw-w64-x86_64-drmingw
    * mingw-w64-i686-tolua / mingw-w64-x86_64-tolua

#. Arch-specific packages for building the client and/or Ruledit (i686 or x86_64, but not both).

    * mingw-w64-i686-qt5 / mingw-w64-x86_64-qt5
    * mingw-w64-i686-qt5-svg / mingw-w64-x86_64-qt5-svg
    * mingq-w64-i686-karchive-qt5 / mingw-w64-x86_64-karchive-qt5

#. Arch-specific package needed for building Windows installer package (i686 or x86_64, but not both).

    * mingw-w64-i686-nsis / mingw-w64-x86_64-nsis

#. Add some environment variables to the ``.bash_profile`` file. The code sample assumes x86_64.

.. code-block:: sh

  export PATH=/mingw64/bin:${PATH}
  export MSYSTEM=MINGW64
  export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig


Premade Environment
===================

Visit https://github.com/jwrober/freeciv-msys2 to get a set of scripts and instructions to quickly build an
environment. The scripts create an x86_64 environment.


Build
=====

Now that you have the environment setup. You can follow the steps in :doc:`/Getting/install`. MSYS2 in a
Linux emulation evironment on Windows, so the commands for :file:`git`, :file:`cmake`, etc. work the same.
