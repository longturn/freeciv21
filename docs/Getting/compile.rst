..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 louis94 <m_louis30@yahoo.com>

Compiling Freeciv21
*******************

Freeciv21 has a number of prerequisites. Note, that apart from the first prerequisite, the Freeciv21
configuration process is smart enough to work out whether your system is suitable. If in doubt, just try it.

An operating system that support Qt
    Any modern operating system that supports Qt 5.15+ is required. As of this writing this is Linux,
    Microsoft Windows\ |reg| and Apple macOS\ |reg|.

    Linux Distributions:

    * Arch
    * Debian 11+ (Bullseye)\ |reg|
    * Fedora 30+
    * Gentoo
    * KDE Neon
    * Manjaro
    * Mint 20+ or Mint Debian Edition (set to Bullseye)
    * openSUSE 15.3+
    * Slackware
    * Ubuntu 22.04 LTS+


.. note::
  The above list of Linux distributions is, of course, not exhaustive. The Freeciv21 Community has simply
  listed the mainline, well supported, distributions here. The code repository has Continuous Integration
  enabled and all code commits pass through Ubuntu, macOS and Windows for testing. It is assumed that the
  user is keeping his/her computer OS up to date. Support by the community for these distributions will be
  better than for some of the others out there, so keep that in mind if you are not an experienced Linux user.

.. note::
  The following instructions on this page are for Linux, MSYS2 and macOS environments. You will need to
  :doc:`install MSYS2 </Contributing/msys2>` first before continuing here if using MSYS2 on Windows.
  However, you can also compile on Windows with Microsoft
  :doc:`Visual Studio </Contributing/visual-studio>`. The Visual Studio instructions are self contained.
  No need to return to this page after following the installation instructions.


A C and C++ compiler
    Freeciv21 is written in very portable C and C++. Both 32 and 64-bit machines are supported. You cannot
    use a "K&R C" compiler. The C++ compiler must support C++ 17.

    Development of Freeciv21 is primarily done with :file:`gcc`, the GNU project's excellent C and C++
    compiler. For complete cross-platform support the Longturn community uses the LLVM project's
    :file:`clang-cl` compiler, which is supported on Linux, Windows and macOS.

The Cmake program
    Freeciv21 developers generally use :file:`cmake`, the Kitware make program. You can check if you have
    :file:`cmake` installed on your system by typing the following command. The output should include
    "Kitware cmake" somewhere and the version should be >=3.12.

.. code-block:: sh

  $ cmake --version


The Ninja cmake build program
    Freeciv21 uses the :file:`ninja` build tool. You can check if you have :file:`ninja` installed on your
    system by typing the following command. The output should include :file:`ninja` version >=1.10.

.. code-block:: sh

  $ ninja --version


GNU Libtool
    GNU Libtool is a generic library support script that hides the complexity of using shared libraries
    behind a consistent, portable interface. Freeciv21 requires version 2.2 or better.

    https://www.gnu.org/software/libtool/

SQLite
    SQLite is a C-language library that implements a small, fast, self-contained, high-reliability,
    full-featured, SQL database engine. SQLite is the most used database engine in the world. SQLite is
    built into all mobile phones and most computers and comes bundled inside countless other applications
    that people use every day. Freeciv21 requires version 3.

    http://www.sqlite.org/

GNU Gettext
    GNU Gettext is used for Internationalization support. Freeciv21 requires version 0.15 or better. The
    :file:`xgettext` program is required to create the :literal:`*.gmo` files which are not
    included in the git tree.

    https://www.gnu.org/software/gettext/

Lua
    Lua is a powerful, efficient, lightweight, embedable scripting language. It supports procedural
    programming, object-oriented programming, functional programming, data-driven programming, and data
    description. Exact version 5.3 is preferred.

    https://www.lua.org/about.html

KF 5 Archive Library
    KArchive provides classes for easy reading, creation and manipulation of "archive" formats like ZIP
    and TAR.

SDL2_Mixer
    SDL_mixer is a sample multi-channel audio mixer library.

Python
    Freeciv21 requires version 3 of Python


Prerequisites for the Client and Tools
======================================

The Freeciv21 project maintains a single Qt based client.

C++ compiler.
    The client is written in C++, so you need an appropriate compiler. In Freeciv21 development, :file:`g++`
    has been used as well as tests against LLVM's compiler (:file:`clang++`)

QT Libraries
    Freeciv21 uses the Qt libraries, specifically :file:`Qt5Core`, :file:`Qt5Gui`, :file:`Qt5Network`,
    :file:`Qt5Svg`, and :file:`Qt5Widgets` libraries and headers.

    At least version 5.15 is required.

Installing Package Dependencies
===============================

See the `Debian Linux Packages`_ section below on the steps to install the components for Debian Linux and
its variants.

See the `macOS Packages`_ section below on the steps to install the components for Apple macOS.

If you are running Windows and want to use the MSYS2 environment and have not set it up yet, then
:doc:`do so now </Contributing/msys2>`, before continuing.

Lastly, if you are running Windows and want to use Visual Studio, you can follow the Microsoft
:doc:`Visual Studio </Contributing/visual-studio>` instructions. The Visual Studio instructions are
self contained. You do not need to return here in that case.

Debian Linux Packages
=====================
Below are all the command line steps needed to start with a fresh install of Debian or its variants (e.g.
Ubuntu, Linux Mint) to install Freeciv21.

Start with ensuring you have a source repository (:file:`deb-src`) turned on in apt sources and then run the
following commands:

.. code-block:: sh

  $ sudo apt update

  $ sudo apt install git \
     cmake \
     ninja-build \
     python3 \
     python3-pip \
     qtbase5-dev \
     libqt5svg5-dev \
     libkf5archive-dev \
     liblua5.3-dev \
     libsqlite3-dev \
     libsdl2-mixer-dev \
     libmagickwand-dev \
     libunwind-dev \
     libdw-dev \
     python3-sphinx \
     clang-format-11


At this point, follow the steps in `Obtaining the Source Code`_ section below.

macOS Packages
==============

Below are all the command line steps needed to start with a fresh install of macOS.

.. code-block:: sh

  $ brew update

  $ brew install \
      cmake \
      ninja \
      python3 \
      gettext \
      vcpkg
      brew link gettext --force

  $ export VCPKG_ROOT="$HOME/vcpkg"


Obtaining the Source Code
=========================

In order to compile Freeciv21, you need a local copy of the source code. You can download a saved version in
an archive file (:file:`.tar.gz` or :file:`.zip`) of the code from the project releases page at
https://github.com/longturn/freeciv21/releases. Alternately you can get the latest from the master branch with
the :file:`git` program with this command:

.. code-block:: sh

  $ git clone https://github.com/longturn/freeciv21.git
  $ cd freeciv21


Configuring
===========

Configuring Freeciv21 for compilation requires the use of the :file:`cmake` program.

On Debian Linux, to build with defaults enter the following command from the freeciv21 directory. Continue
reading in the `Other CMake Notes`_ section below for more notes about other command line options you can give
:file:`cmake`.

.. code-block:: sh

  $ cmake . -B build -G Ninja


On macOS, you need to use a preset that is defined in the :file:`CMakePresets.json` file. When complete
you can go to the `Compiling/Building`_ section below to continue.

.. code-block:: sh

  $ cmake --preset fullrelease-macos -S . -B build

.. note::
  The first time you run the this command, :file:`cmake` invokes the :file:`vcpkg` installation process to
  download and compile all of the project dependencies listed in the manifest file: :file:`vcpkg.json`.
  :strong:`This will take a very long time`. On a fast computer with a good Internet connection it will take
  at least 3 hours to complete. Everything will be downloaded and compiled into the :file:`$HOME/vcpkg`
  directory. Binaries for the packages will be copied into the :file:`./build/` directory inside of the main
  Freeciv21 directory and reused for subsequent builds.


Compiling/Building
==================

Once the build files have been written, then compile with this command:

.. code-block:: sh

  $ cmake --build build


Installing
==========

Once the compilation is complete, install the game with this command.

.. code-block:: sh

  $ cmake --build build --target install


.. note:: If you did not change the default install prefix, you will need to elevate privileges
    with :file:`sudo`.

.. tip:: If you want to enable menu integration for the installed copy of Freeciv21, you will want
    to copy the :literal:`.desktop` files in :file:`$CMAKE_INSTALL_PREFIX/share/applications` to
    :file:`$HOME/.local/share/applications`.

    This is only necessary if you change the installation prefix. If you do not and use elevated
    privileges, then the files get copied to the system default location.

At this point, the compilation and installation process is complete. The following sections document other
aspects of the packaging and documentation generation process.

Debian and Windows Package Notes
================================

Operating System native packages can be generated for Debian and Windows based systems.

Debian
------

Assuming you have obtained the source code and installed the package dependencies in the sections above,
follow these steps to generate the Debian package:

.. code-block:: sh

  $ cmake --build build --target package


When the last command is finished running, you will find a :file:`.deb` installer in
:file:`build/Linux-${arch}`

Microsoft Windows
-----------------

There are two platforms available for installing Freeciv21 on Windows: :doc:`MSYS2 <../Contributing/msys2>`
and :doc:`Visual Studio <../Contributing/visual-studio>`. The package target is only supported on MSYS2 due to
licensing `constraints <https://www.gnu.org/licenses/gpl-faq.en.html#WindowsRuntimeAndGPL>`_.

Once your MSYS2 environment is ready, start with `Obtaining the Source Code`_ above. Instead of installing,
use this command to create the Windows Installer package:

.. code-block:: sh

  $ cmake --build build --target package


When the command is finished running, you will find an installer in :file:`build/Windows-${arch}`

Documentation Build Notes
=========================

Freeciv21 uses :file:`python3-sphynx` and https://readthedocs.org/ to generate the well formatted HTML
documentation that you are reading right now. To generate a local copy of the documentation from the
:file:`docs` directory you need two dependencies and a special build target.

The Sphinx Build Program
    The :file:`sphinx-build` program is used to generate the documentation from reStructuredText files
    (:file:`*.rst`).

    https://www.sphinx-doc.org/en/master/index.html

ReadTheDocs Theme
    Freeciv21 uses the Read The Docs (RTD) theme for the general look and feel of the documentation.

    https://sphinx-rtd-theme.readthedocs.io/en/stable/

If you are running Debian Linux, the base program is installed by the instructions in the
`Debian Linux Packages`_ section above. The documentation is not built by default from the steps in
`Compiling/Building`_ above. To generate a local copy of the documentation, issue this command:

.. code-block:: sh

  $ cmake --build build --target docs


Other CMake Notes
=================

To customize the compile, :file:`cmake` requires the use of command line parameters. :file:`cmake` calls
them directives and they start with :literal:`-D`. The defaults are marked with :strong:`bold` text.

=========================================== =================
Directive                                    Description
=========================================== =================
FREECIV_ENABLE_TOOLS={:strong:`ON`/OFF}     Enables all the tools with one parameter (Ruledit, FCMP,
                                            Ruleup, and Manual)
FREECIV_ENABLE_SERVER={:strong:`ON`/OFF}    Enables the server. Should typically set to ON to be able
                                            to play AI games
FREECIV_ENABLE_NLS={:strong:`ON`/OFF}       Enables Native Language Support
FREECIV_ENABLE_CIVMANUAL={:strong:`ON`/OFF} Enables the Freeciv21 Manual application
FREECIV_ENABLE_CLIENT={:strong:`ON`/OFF}    Enables the Qt client. Should typically set to ON unless you
                                            only want the server
FREECIV_ENABLE_FCMP_CLI={ON/OFF}            Enables the command line version of the Freeciv21 Modpack
                                            Installer
FREECIV_ENABLE_FCMP_QT={ON/OFF}             Enables the Qt version of the Freeciv21 Modpack Installer
                                            (recommended)
FREECIV_ENABLE_RULEDIT={ON/OFF}             Enables the Ruleset Editor
FREECIV_ENABLE_RULEUP={ON/OFF}              Enables the Ruleset upgrade tool
FREECIV_USE_VCPKG={ON/:strong:`OFF`}        Enables the use of VCPKG
FREECIV_DOWNLOAD_FONTS{:strong:`ON`/OFF}    Enables the downloading of Libertinus Fonts
CMAKE_BUILD_TYPE={:strong:`Release`/Debug}  Changes the Build Type. Most people will pick Release
CMAKE_INSTALL_PREFIX=/some/path             Allows an alternative install path. Default is
                                            :file:`/usr/local/freeciv21`
=========================================== =================

For more information on other cmake directives see
https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html.

Once the command line directives are determined, the appropriate command looks like this:

.. code-block:: sh

  $ cmake . -B build -G Ninja \
     -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=$HOME/Install/Freeciv21


A very common Debian Linux configuration command looks like this:

.. code-block:: sh

  $ cmake . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=$PWD/build/install


.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
