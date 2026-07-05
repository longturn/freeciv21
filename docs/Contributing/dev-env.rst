.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Set up a Development Environment
********************************

Contributing code to the Freeciv21 project or contributing to any of the games that the Longturn community
manages requires a bit of setup. This document should get you up and running with the basics. It should be
noted, that you typically only have to go through this process once unless you setup a new workstation.

Base Workstation
================

Freeciv21 can be developed on Linux, Microsoft\ |reg| Windows, and Apple\ |reg| macOS. Any current version of
these OS's is acceptable.

It should be generally understood that :strong:`Linux` is the preferred development platform. For a complete 
set of installation steps, you can refer to :doc:`/Getting/compile`.

* For Windows, you will need to setup a MSYS2 environment first. Refer to :doc:`/Contributing/dev-env-msys2`.

* For macOS, you will need to setup some pre-requisites first. Refer to :doc:`/Contributing/dev-env-macos`.

Technically all you need is a text editor of some kind to edit the files, but most people prefer to use an
integrated development environment (IDE). Freeciv21 recommends Microsoft\ |reg| VS Code. It is a very
capable cross-platform IDE that supports all three OS's.

Start by downloading VS Code from: https://code.visualstudio.com/download.

.. _dev-env-github:

GitHub
======

The Longturn Community uses the online source code control and revision system known as 
`GitHub <https://github.com/>`_. To contribute, you will need an account on this platform. There is no cost.

With an account, you can go to the `Longturn <https://github.com/longturn>`_ community repository page and
:strong:`fork` a repository (such as the Freeciv21 repository) to your personal GitHub account. Go to the main
page of the repository you want to fork and you will find a :strong:`fork` button in the upper-right corner.

In order to push code to the forked repository from your local workstation, you need to setup an SSH key pair
to share with GitHub. Follow these
`instructions <https://docs.github.com/en/authentication/connecting-to-github-with-ssh>`_.

With SSH set up, now it is time to clone the forked repository from your personal GitHub account to a local
copy on your workstation.

First make a working directory to place the files in:

.. code-block:: sh

  $ mkdir -p $HOME/GitHub

  $ cd $HOME/GitHub


You can get the appropriate path by going to your forked copy in a browser, click the code button and then
select the SSH option as shown in this sample screenshot:

.. GitHub Clone SSH:
.. figure:: /_static/images/github_clone_ssh.png
    :align: center
    :height: 250
    :alt: GitHub Clone SSH

    GitHub Clone SSH


Once you have the proper path, here is the command to clone the repository:

.. code-block:: sh

  ~/GitHub$ git clone git@github.com:[username]/freeciv21.git


This will clone the forked repository to the :file:`~/GitHub/freeciv21` directory on your computer.

The final repository setup item is to link the original Longturn project repository to your local area on
your computer:

.. code-block:: sh

  ~/GitHub/freeciv21$ git remote add upstream https://github.com/longturn/freeciv21.git
  ~/GitHub/freeciv21$ git fetch upstream
  ~/GitHub/freeciv21$ git pull upstream master


You will also need to set a couple global configuration settings so :code:`git` knows a bit more about you.

.. code-block:: sh

  ~/GitHub/freeciv21$ git config --global user.email "[email address associated with GitHub]"
  ~/GitHub/freeciv21$ git config --global user.name "[your first and last name]"
  
  
VS Code Setup
=============

VS Code has a huge array of extensions to enhance the usability of the product. While you do not need to 
install any extensions to edit Freeciv21 code, some are useful.

* :strong:`C/C++ Extension Pack from Microsoft`: This extension pack installs 4 extensions: C/C++, 
  C/C++ Themes, C/C++ DevTools, and CMake Tools. These extensions make working with the code much easier and
  integrates our CMake build system into the IDE. You can install it by running
  :code:`ext install ms-vscode.cpptools-extension-pack` from the command bar at the top.

* :strong:`Python from Microsoft`: This extension pack installs 3 extensions: Pylance, Python Debugger, and
  Python Environments. These extensions make working with the Python code (see 
  :file:`common/generate_packets.py`) much easier. You can install it by running 
  :code:`ext install ms-python.python` from the command bar at the top.

* :strong:`YAML by RedHat`: This extension helps with editing the YAML files used by GitHub actions
  (see :file:`.github/workflows/`). You can install it by running :code:`ext install redhat.vscode-yaml` from 
  the command bar at the top.

* :strong:`reStructuredText by Lextudio`: This extension helps with editing the reStructuredText used by our
  documentation system (see :file:`docs/`). You can install it by running 
  :code:`ext install lextudio.restructuredtext` from the command bar at the top. We typically format 
  word-wrap for our documentation around column 110. If you want to add a vertical bar at that point, you can 
  add :code:`"editor.rulers": [110]` to the :file:`settings.json` file.

* :strong:`GitHub Pull Requests by GitHub`: This extension allows you to work with
  :doc:`Pull Requests </Contributing/pull-request>` and :doc:`Issues </Contributing/bugs>` within
  the IDE. You can install it by running :code:`ext install github.vscode-pull-request-github` from the 
  command bar at the top.

Windows MSYS2 Integration
-------------------------

The built-in terminal in VS Code works natively on Linux and macOS. On Windows we need to do some
customization to get the terminal to interact with MSYS2's terminal. While we are integrating the terminal
we can enhance support for :file:`.cpp` code analysis.

Open the :file:`settings.json` from :menuselection:`File --> Preferences --> Settings` and then clicking
on the icon for the settings file (upper right corner). Add this code and adjust the path to where you
installed MSYS2 from :doc:`/Contributing/dev-env-msys2`.

.. code-block:: json

  "terminal.integrated.profiles.windows": {
  "MSYS2 CLANG64": {
    "path": "C:\\Tools\\msys64\\usr\\bin\\bash.exe",
    "args": ["--login", "-i"],
    "env": { "CHERE_INVOKING": "1" }
    }
  },
  "terminal.integrated.defaultProfile.windows": "MSYS2 CLANG64"


While in the :file:`settings.json`, add these lines to configure clang integration:

.. code-block:: json

  "C_Cpp.clang_format_path": "C:\\Tools\\msys64\\clang64\\bin\\clang-format.exe",
  "C_Cpp.formatting": "clangFormat",
  "C_Cpp.codeAnalysis.clangTidy.enabled": true,
  "C_Cpp.codeAnalysis.clangTidy.path": "C:\\Tools\\msys64\\clang64\\bin\\clang-tidy.exe",
  "C_Cpp.default.compilerPath": "C:\\Tools\\msys64\\clang64\\bin\\clang++.exe",
  "C_Cpp.default.intelliSenseMode": "clang-x64",
  "C_Cpp.default.cppStandard": "c++17",
  "C_Cpp.default.cStandard": "c99",
  "C_Cpp.default.includePath": [
    "${workspaceRoot}",
    "${workspaceFolder}\\**",
    "C:\\Tools\\msys64\\clang64\\include",
    "C:\\Tools\\msys64\\clang64\\include\\**",
    "C:\\Tools\\msys64\\clang64\\lib",
    "C:\\Tools\\msys64\\clang64\\lib\\**",
    "C:\\Tools\\msys64\\clang64\\bin"
  ]


Qt Designer
===========

Freeciv21 uses the Qt framework for many things, especially the game client. There are many :file:`.ui` files
in the :file:`client` directory. If you want to edit those in a user interface (versus editing the raw XML),
you will want to install Qt Designer. 

On Linux, you can install Qt Designer from Flathub or as a Snap package. Qt only offers Designer as a 
native package inside the full Qt Creator IDE package, which is overkill.

On Windows and macOS you can get Designer by doing a custom install from here:
https://www.qt.io/development/download-qt-installer-oss. Similar to Linux, you do not need Creator IDE, just
Designer.

You can read the Qt Designer Manual for more help here: https://doc.qt.io/qtdesignstudio/index.html
