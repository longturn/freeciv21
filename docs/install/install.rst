Linux Compiling and Installing
==============================

This page contains sections as follows:

.. contents::
    :local:

General Prerequisites
*********************

Freeciv21 has a number of prerequisites.  Note, that apart from the first prerequisite, the Freeciv21
configuration process is smart enough to work out whether your system is suitable. If in doubt, just try it.

* An operating system that support Qt.

  Any modern operating system that support Qt is required. As of this writing this is Linux, Microsoft
  Windows\ |reg| and Apple Mac OS X\ |reg|. On Windows Msys2 is supported.

* A C and C++ 14 compiler.

  Freeciv21 is written in very portable C and C++. Both 32- and 64-bit machines are supported. You cannot
  use a "K&R C" compiler, or a C++ compiler. The C++ compiler must support C++ 17.

  Development of Freeciv21 is primarily done with :file:`gcc`, the GNU project's excellent C and C++
  compiler. Microsoft Windows MS Visual C support is under development.

* The :file:`cmake` program.

  Freeciv21 developers generally use :file:`cmake`, the Kitware make program. You can check if you have
  :file:`cmake` installed on your system by typing the following command. The output should include "Kitware
  cmake" somewhere and the version should be >=3.12.

.. code-block:: rst

  $ cmake --version


* The Ninja cmake build program


* :file:`libtool`

  version 2.2 or better

* :file:`libsqlite3`

  http://www.sqlite.org/

* The programs from GNU :file:`gettext` version 0.15 or better

  Especially the :file:`xgettext` program is required to create the :literal:`*.gmo` files which aren't
  included in the git tree.

* Lua version 5.3 or higher

  Exact 5.3 is preferred.

* KF 5 Archive Library

  KArchive provides classes for easy reading, creation and manipulation of "archive" formats like ZIP and TAR.
  
* SDL2_Mixer

  SDL_mixer is a sample multi-channel audio mixer library.

* Python 3


Prerequisites for the client and tools
**************************************

The Freeciv21 project maintains a single Qt client.

* C++ compiler.

  The client is written in C++, so you need an appropriate compiler. In Freeciv21 development, :file:`g++`
  has been used as well as tests against LLVM's compiler (:file:`clang++`)

* :file:`Qt5Core`, :file:`Qt5Gui`, and :file:`Qt5Widgets` libraries and headers.

  At least version 5.11 is required.


Obtaining the Source Code
*************************

In order to compile Freeciv21, you need a local copy of the source code. You can download a saved version of 
the code from the project releases page at https://github.com/longturn/freeciv21/releases. Alternately you 
can get the latest from the master branch with the :file:`git` program with this command:

.. code-block:: rst

  $ git clone https://github.com/longturn/freeciv21.git


.. _configuring:

Configuring
***********

Configuring Freeciv21 for compilation requires the use of the :file:`cmake` program. To build with defaults enter the following commmand from the freeciv21 directory:

.. code-block:: rst

  $ cmake . -B build -G Ninja


To customize the compile, :file:`cmake` requires the use of command line parameters. :file:`cmake` calls 
them directives and they start with :literal:`-D`. The defaults are marked with :strong:`bold` text.

============================================ =================
Directive                                    Description
============================================ =================
DFREECIV_ENABLE_TOOLS={:strong:`ON`/OFF}     Enables all the tools with one parameter (Ruledit, FCMP, 
                                             Ruleup, and Manual)
DFREECIV_ENABLE_SERVER={:strong:`ON`/OFF}    Enables the server. Should typically set to ON to be able 
                                             to play AI games
DFREECIV_ENABLE_NLS={:strong:`ON`/OFF}       Enables Native Language Support
DFREECIV_ENABLE_CIVMANUAL={:strong:`ON`/OFF} Enables the Freeciv Manual application
DFREECIV_ENABLE_CLIENT={:strong:`ON`/OFF}    Enables the Qt client. Should typically set to ON unless you 
                                             only want the server
DFREECIV_ENABLE_FCMP_CLI={ON/OFF}            Enables the command line version of the Freeciv21 Modpack 
                                             Installer
DFREECIV_ENABLE_FCMP_QT={ON/OFF}             Enables the Qt version of the Freeciv21 Modpack Installer 
                                             (recommended)
DFREECIV_ENABLE_RULEDIT={ON/OFF}             Enables the Ruleset Editor
DFREECIV_ENABLE_RULEUP={ON/OFF}              Enables the Ruleset upgrade tool
DCMAKE_BUILD_TYPE={:strong:`Release`/Debug}  Changes the Build Type. Most people will pick Release
DCMAKE_INSTALL_PREFIX=/some/path             Allows an alternative install path. Default is 
                                             :file:`/usr/local/share/freeciv21`
============================================ =================

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
******************

Once the build files have been written, then compile with this command:

.. code-block:: rst

  $ cmake --build build


Installing
**********

Once the compilation is complete, install the game with this command.

.. code-block:: rst

  $ cmake --build build --target install


.. note:: If you did not change the default install prefix, you will need to elevate privileges 
    with :file:`sudo`.

After compilation, the important results are:

  - The :file:`build/freeciv21-client` client application binary.
  - The :file:`build/freeciv21-server` game server binary.


Debian Linux Notes
******************

Below are all the command line steps needed to start with a fresh install of Debian or its variants (e.g.
Ubuntu, Linux Mint) to install Freeciv21.

Start with ensuring your have a source repository (deb-src) turned on in apt sources and then run the
following commands.

.. code-block:: rst

  $ sudo apt update

  $ sudo apt build-dep freeciv

  $ sudo apt install git \
     cmake \
     ninja-build \
     python3 \
     qt5-default \
     libkf5archive-dev \
     liblua5.3-dev \
     libmagickwand-dev \
     libsdl2-mixer-dev \
     libunwind-dev \
     libdw-dev

  $ mkdir -p $HOME/GitHub

  $ cd $HOME/GitHub

  $ git clone https://github.com/longturn/freeciv21.git

  $ cd freeciv21

At this point follow the steps in the configuring_ section above.


Windows Notes
*************

Msys2 is an available environment for compiling Freeciv21. Microsoft Windows Visual C is under development.

Freeciv21 currently supports building and installing using the Msys2 environment. Build instructions for
Msys2 versions are documented in :file:`doc/README.msys2`. Alternately you can visit
https://github.com/jwrober/freeciv-msys2 for ready made scripts.

Follow the steps starting in configuring_ above.

Instead of installing, use this command to create the Windows Installer package:

.. code-block:: rst

  $ cmake --build build --target package

When the Ninja command is finished running, you will find an installer in :file:`build/Windows-${arch}`

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
