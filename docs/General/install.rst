Compiling and Installing
************************

General Prerequisites
=====================

Freeciv21 has a number of prerequisites.  Note, that apart from the first prerequisite, the Freeciv21
configuration process is smart enough to work out whether your system is suitable. If in doubt, just try it.

An operating system that support Qt
    Any modern operating system that supports Qt 5.12+ is required. As of this writing this is Linux, Microsoft
    Windows\ |reg| and Apple Mac OS X\ |reg|. On Windows MSYS2 (MingW) is required.

    Linux Distributions:

    * Arch
    * CentOS 8+
    * Debian 11+ (Bullseye)
    * Fedora 28+
    * Gentoo
    * KDE Neon
    * Manjaro
    * Mint 20+ or Mint Debian Edition
    * openSUSE 15.2+
    * Slackware
    * Ubuntu 20.04 LTS+


.. note::
  The above list of Linux distributions is, of course, not exhaustive. The Freeciv21 Community has simply
  listed the mainline, well supported, distributions here. The code repository has Continuous Integration
  enabled and all code commits pass through Ubuntu, Mac OS and Windows for testing. It is assummed that the
  user is keeping his/her computer OS up to date. Support by the community for these distributions will be
  better than for some of the others out there, so keep that in mind if you are not an experienced Linux user.


A C and C++ compiler
    Freeciv21 is written in very portable C and C++. Both 32- and 64-bit machines are supported. You cannot
    use a "K&R C" compiler. The C++ compiler must support C++ 17.

    Development of Freeciv21 is primarily done with :file:`gcc`, the GNU project's excellent C and C++
    compiler. Microsoft Windows MS Visual C support is under development.

The Cmake program
    Freeciv21 developers generally use :file:`cmake`, the Kitware make program. You can check if you have
    :file:`cmake` installed on your system by typing the following command. The output should include
    "Kitware cmake" somewhere and the version should be >=3.12.

.. code-block:: rst

  $ cmake --version


The Ninja cmake build program
    Freeciv21 uses the :file:`ninja` build tool. You can check if you have :file:`ninja` installed on your
    system by typing the following command. The output should include :file:`ninja` version >=1.10.

.. code-block:: rst

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
    :file:`xgettext` program is required to create the :literal:`*.gmo` files which aren't
    included in the git tree.

    https://www.gnu.org/software/gettext/

Lua
    Lua is a powerful, efficient, lightweight, embeddable scripting language. It supports procedural
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

The Freeciv21 project maintains a single Qt client.

C++ compiler.
    The client is written in C++, so you need an appropriate compiler. In Freeciv21 development, :file:`g++`
    has been used as well as tests against LLVM's compiler (:file:`clang++`)

QT Libraries
    Freeciv21 uses the Qt libraries, specifically :file:`Qt5Core`, :file:`Qt5Gui`, :file:`Qt5Network`,
    :file:`Qt5Svg`, and :file:`Qt5Widgets` libraries and headers.

    At least version 5.11 is required.


Obtaining the Source Code
=========================

In order to compile Freeciv21, you need a local copy of the source code. You can download a saved version of
the code from the project releases page at https://github.com/longturn/freeciv21/releases. Alternately you
can get the latest from the master branch with the :file:`git` program with this command:

.. code-block:: rst

  $ git clone https://github.com/longturn/freeciv21.git


Configuring
===========

Configuring Freeciv21 for compilation requires the use of the :file:`cmake` program. To build with defaults
enter the following commmand from the freeciv21 directory:

.. code-block:: rst

  $ cmake . -B build -G Ninja


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
FREECIV_ENABLE_CIVMANUAL={:strong:`ON`/OFF} Enables the Freeciv Manual application
FREECIV_ENABLE_CLIENT={:strong:`ON`/OFF}    Enables the Qt client. Should typically set to ON unless you
                                            only want the server
FREECIV_ENABLE_FCMP_CLI={ON/OFF}            Enables the command line version of the Freeciv21 Modpack
                                            Installer
FREECIV_ENABLE_FCMP_QT={ON/OFF}             Enables the Qt version of the Freeciv21 Modpack Installer
                                            (recommended)
FREECIV_ENABLE_RULEDIT={ON/OFF}             Enables the Ruleset Editor
FREECIV_ENABLE_RULEUP={ON/OFF}              Enables the Ruleset upgrade tool
CMAKE_BUILD_TYPE={:strong:`Release`/Debug}  Changes the Build Type. Most people will pick Release
CMAKE_INSTALL_PREFIX=/some/path             Allows an alternative install path. Default is
                                            :file:`/usr/local/freeciv21`
=========================================== =================

For more information on other cmake directives see
https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html.

Once the command line directives are determined, the appropriate command looks like this:

.. code-block:: rst

  $ cmake . -B build -G Ninja \
     -DFREECIV_ENABLE_TOOLS=OFF \
     -DFREECIV_ENABLE_SERVER=ON \
     -DCMAKE_BUILD_TYPE=Release \
     -DFREECIV_ENABLE_NLS=OFF \
     -DCMAKE_INSTALL_PREFIX=$HOME/Install/Freeciv21


Compiling/Building
==================

Once the build files have been written, then compile with this command:

.. code-block:: rst

  $ cmake --build build


Installing
==========

Once the compilation is complete, install the game with this command.

.. code-block:: rst

  $ cmake --build build --target install


.. note:: If you did not change the default install prefix, you will need to elevate privileges
    with :file:`sudo`.

.. tip:: If you want to enable menu integration for the installed copy of Freeciv21, you will want
    to copy the :literal:`.desktop` files in :file:`$CMAKE_INSTALL_PREFIX/share/applications` to
    :file:`$HOME/.local/share/applications`.

    This is only necessary if you change the installation prefix. If you don't and use elevated
    privileges, then the files get copied to the system default location.


Debian Linux Notes
==================

Below are all the command line steps needed to start with a fresh install of Debian or its variants (e.g.
Ubuntu, Linux Mint) to install Freeciv21.

Start with ensuring you have a source repository (:file:`deb-src`) turned on in apt sources and then run the
following commands.

.. code-block:: rst

  $ sudo apt update

  $ sudo apt build-dep freeciv

  $ sudo apt install git \
     cmake \
     ninja-build \
     python3 \
     python3-pip \
     qt5-default \
     libkf5archive-dev \
     liblua5.3-dev \
     libmagickwand-dev \
     libsdl2-mixer-dev \
     libunwind-dev \
     libdw-dev \
     python3-sphinx \
     clang-format-11

  $ pip install sphinx_rtd_theme

  $ mkdir -p $HOME/GitHub

  $ cd $HOME/GitHub

  $ git clone https://github.com/longturn/freeciv21.git

  $ cd freeciv21

At this point follow the steps in the configuring_ section above.


Windows Notes
=============

Msys2 is an available environment for compiling Freeciv21. Microsoft Windows Visual C is under development.

Freeciv21 currently supports building and installing using the Msys2 environment. Build instructions for
Msys2 versions are documented in :doc:`../Contributing/msys2`. Alternately you can visit
https://github.com/jwrober/freeciv-msys2 for ready made scripts.

Follow the steps starting in configuring_ above.

Instead of installing, use this command to create the Windows Installer package:

.. code-block:: rst

  $ cmake --build build --target package


When the Ninja command is finished running, you will find an installer in :file:`build/Windows-${arch}`

Documentation Build Notes
=========================

Freeciv21 uses :file:`python3-sphynx` and https://readthedocs.org/ to generate well formatted HTML
documentation. To generate a local copy of the documentation from the :file:`docs` directory you need two
dependencies and a special build target.

The Sphinx Build Program
    The :file:`sphinx-build` program is used to generate the documentation from reStructuredText files
    (:file:`*.rst`).

    https://www.sphinx-doc.org/en/master/index.html

ReadTheDocs Theme
    Freeciv21 uses the Read The Docs (RTD) theme for the general look and feel of the documentation.

    https://sphinx-rtd-theme.readthedocs.io/en/stable/

The documentation is not built by default from the steps in `Compiling/Building`_ above. To generate a local
copy of the documentation, issue this command:

.. code-block:: rst

  $ cmake --build build --target docs


.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
