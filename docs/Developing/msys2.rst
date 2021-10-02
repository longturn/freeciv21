MSYS2 Windows Installer Build
*****************************

This document is about building compiling Freeciv21 and building the Windows Installer packages using MSYS2
from http://msys2.github.io


Setup
=====

This chapter is about creating new msys2 build environment.

#. Install MSYS2 following the documentation on their homepage

#. Download: https://sourceforge.net/projects/msys2/files/Base/x86_64/msys2-x86_64-20210228.exe for win64 host

#. Run it to install MSYS2 on build system

#. Launch :file:`msys2_shell` to update packages

.. code-block:: rst

    > pacman -Syu


#. Install following packages with :code:`pacman -Su`

#. Packages needed for building freeciv21. These packages are needed even if you don't plan to make the
installer, but only build Freeciv21 for local use.

#. Arch independent packages needed for building Freeciv21

#. Arch independent packages always needed for building Freeciv21. With these packages it's possible to build
   Freeciv21 source tree that has already such generated files present that are created for the release tarball.

* make
* gettext
* pkg-config

#. Arch independent packages needed for getting and extracting Freeciv21 source tarball

* tar

#. Arch independent packages needed for building Freeciv21 from repository checkout. These are needed in
   addition to the ones always needed for building Freeciv21.

* git

#. Arch-specific packages for building common parts (i686 or x86_64, not both)

* mingw-w64-i686-gcc / mingw-w64-x86_64-gcc
* mingw-w64-i686-curl / mingw-w64-x86_64-curl
* mingw-w64-i686-bzip2 / mingw-w64-x86_64-bzip2
* mingw-w64-i686-readline / mingw-w64-x86_64-readline
* mingw-w64-i686-lua / mingw-w64-x86_64-lua
* mingw-w64-i686-imagemagick / mingw-w64-x86_64-imagemagick
* mingw-w64-i686-SDL2_mixer / mingw-w64-x86_64-SDL2_mixer
* mingw-w64-i686-cmake / mingw-w64-x86_64-cmake
* mingw-w64-i686-ninga / mingw-w64-x86_64-ninja
* mingw-w64-i686-libunwind / mingw-w64-x86_64-libunwind

#. Arch-specific optional packages for building common parts (i686 or x86_64, not both)

* mingw-w64-i686-drmingw / mingw-w64-x86_64-drmingw
* mingw-w64-i686-tolua / mingw-w64-x86_64-tolua

#. Arch-specific packages for buildind Qt-client and/or Ruledit (i686 or x86_64, not both)

* mingw-w64-i686-qt5 / mingw-w64-x86_64-qt5
* mingq-w64-i686-karchive-qt5 / mingw-w64-x86_64-karchive-qt5

#. Arch-specific packages for building sdl2-client (i686 or x86_64, not both)

* mingw-w64-i686-SDL2_image / mingw-w64-x86_64-SDL2_image
* mingw-w64-i686-SDL2_ttf / mingw-w64-x86_64-SDL2_ttf
* mingw-w64-i686-SDL2_gfx / mingw-w64-x86_64-SDL2_gfx

#. Packages needed for building installer package. These are needed in addition to above ones used in the
   building step already.

#. Arch-specific packages needed for building installer package (i686 or x86_64, not both)

* mingw-w64-i686-nsis / mingw-w64-x86_64-nsis


Premade Environment
===================

Visit https://github.com/jwrober/freeciv-msys2 to get a set of scripts and instructions to quickly build an
environment. The scripts create an x86_64 environment.


Build
=====

Now that you have the environment setup. You can follow the steps in :doc:`../General/install`

** END **
