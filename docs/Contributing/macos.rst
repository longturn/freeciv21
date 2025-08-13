.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Setting up macOS for Development
********************************

Freeciv21 compiles and runs on Apple\ |reg| macOS Sonoma (v14) or higher. The following steps can be used to
setup your mac to compile and run Freeciv21.

Homebrew
========

Homebrew is a package manager for macOS and allows us to download and install some libraries and tools we
need. Let us start with getting it installed and setting an environment variable.

.. code-block:: sh

  % /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  % echo '# Freeciv21 macOS variables' >> $HOME/.zshrc
  % echo 'eval "$(/usr/local/bin/brew shellenv)"' >> $HOME/.zshrc
  % source $HOME/.zshrc


.. note::
  These instructions assume you are using the default z-shell (``zsh``) as your terminal shell. If you are
  using the bash shell instead replace the output to :file:`$HOME/.bash_profile` or :file:`$HOME/.bashrc`.

With homebrew installed and setup, now install some packages:

.. code-block:: sh

  % brew install --overwrite \
      autoconf \
      autoconf-archive \
      automake \
      cmake \
      create-dmg \
      libtool \
      llvm@17 \
      ninja \
      pkgconf \
      python@3.13


.. note::
  Every package is required except for ``create-dmg`` and ``llvm@17``. The ``create-dmg`` package is used to
  build an app bundle and package to a :file:`.dmg` installer. The ``llvm@17`` package installs the
  varying ``clang`` tools we use such as ``clang-tidy`` v17. We do not override the version of the ``clang++``
  compiler that is installed with the XCode CLI tools that homebrew installed earlier.


The Python package offers non-versioned programs (``python`` vs ``python3``). To make accessing them easier,
let us adjust the path variable:

.. code-block:: sh

  % echo 'export PATH="$PATH:/usr/local/opt/python@3.13/libexec/bin"' >> $HOME/.zshrc
  % source $HOME/.zshrc


Setup VCPKG
===========

Similar to homebrew, Microsoft\ |reg| manages a package library called vcpkg. Freeciv21 requires libraries from
vcpkg, so we install with these commands:

.. code-block:: sh

  % mkdir $HOME/GitHub
  % cd $HOME/GitHub
  % git clone https://github.com/microsoft/vcpkg
  % echo 'export VCPKG_ROOT="$HOME/GitHub/vcpkg"' >> $HOME/.zshrc
  % source $HOME/.zshrc
  % /bin/bash ./vcpkg/bootstrap-vcpkg.sh


Get Freeciv21
=============

Follow these steps to get Freeciv21 from :ref:`GitHub <dev-env-github>`.

Configure and Build
===================

On macOS, you need to use a preset that is defined in the :file:`CMakePresets.json` file.

.. code-block:: sh

  % cmake --preset fullrelease-macos -S .
  % cmake --build build
  % cmake --build build --target test
  % cmake --build build --target install


.. note::
  The first time you run the first command, :file:`cmake` invokes the :file:`vcpkg` installation process to
  download and compile all of the project dependencies listed in the manifest file: :file:`vcpkg.json`.
  :strong:`This will take a very long time`. On a fast computer with a good Internet connection it will take
  at least 3 hours to complete. Everything will be downloaded and compiled into the :file:`$HOME/GitHub/vcpkg`
  directory. Binaries for the packages will be copied into the :file:`./build/` directory inside of the main
  Freeciv21 directory and reused for subsequent builds.


Create App Bundle and DMG Installer
===================================

Follow these steps to build an app bundle for Freeciv21:

.. code-block:: sh

  % cd build
  % mkdir -pv Freeciv21.app/Contents/MacOS Freeciv21.app/Contents/Resources/doc
  % cp -v dist/Info.plist Freeciv21.app/Contents/
  % cp -rv install/share/doc/freeciv21/* Freeciv21.app/Contents/Resources/doc
  % cp -rv install/share/freeciv21/* Freeciv21.app/Contents/Resources
  % cp -v install/bin/freeciv21-* Freeciv21.app/Contents/MacOS
  % mkdir client.iconset
  % cp -v ../data/icons/16x16/freeciv21-client.png client.iconset/icon_16x16.png
  % cp -v ../data/icons/32x32/freeciv21-client.png client.iconset/icon_16x16@2x.png
  % cp -v ../data/icons/32x32/freeciv21-client.png client.iconset/icon_32x32.png
  % cp -v ../data/icons/64x64/freeciv21-client.png client.iconset/icon_32x32@2x.png
  % cp -v ../data/icons/128x128/freeciv21-client.png client.iconset/icon_128x128.png
  % iconutil -c icns client.iconset
  % cp -v client.icns Freeciv21.app/Contents/Resources


Follow this steps to create the DMG:

.. code-block:: sh

  % create-dmg \
        --hdiutil-verbose \
        --volname "Freeciv21 Game Installer" \
        --volicon "client.icns" \
        --window-pos 200 120 \
        --window-size 800 400 \
        --icon-size 128 \
        --icon "Freeciv21.app" 200 190 \
        --hide-extension "Freeciv21.app" \
        --app-drop-link 600 185 \
        "Freeciv21-installer.dmg" \
        "Freeciv21.app"
