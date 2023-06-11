..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

freeciv21-modpack-qt
********************

SYNOPSIS
========

``freeciv21-modpack-qt`` [ -i|--install `<URL>` ] [ -L|--List `<URL>`] [ -h|--help ] [ -v|--version ]

DESCRIPTION
===========

.. include:: freeciv21-server.rst
  :start-line: 17
  :end-line: 29

This is a program to read and install a modpack URL from the command line. When given no command line
parameters the Qt base GUI will load and allow for modpack installation. For a pure command line modpack
installer, see freeciv21-modpack(6).

OPTIONS
=======

The following options are accepted on the command line of the modpack installer. They may not be combined as
with other tools. For example:
``freeciv21-modpack-qt -Lp https://raw.githubusercontent.com/longturn/modpacks/main/index.json ~/.local/fc21``
will not work. Instead you will need to enter each option separately, such as,
``freeciv21-modpack-qt -L https://raw.githubusercontent.com/longturn/modpacks/main/index.json -p ~/.local/fc21``.

``-d, --debug <LEVEL>``
    Set debug log level (fatal/critical/warning/info/debug). Default log level is ``info``.

``-i, --install <URL>``
    Automatically install modpack from a given URL.

``-L, --List <URL>``
    Load modpack list from given URL.

``-p, --prefix <DIR>``
    Install modpacks to given directory hierarchy. Default location is `~/.local/share/freeciv21`.

``-h, --help``
    Display help on command line options.

``--help-all``
    Display help including Qt specific options.

``-v, --version``
    Display version information.

.. include:: freeciv21-server.rst
  :start-line: 148


