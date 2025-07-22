.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

freeciv21-manual
****************

SYNOPSIS
========

``freeciv21-manual`` [ -l|--log `<FILE>` ] [ -r|--ruleset `<RULESET>` ] [ -h|--help ] [ -v|--version ]

DESCRIPTION
===========

.. include:: freeciv21-server.rst
  :start-line: 17
  :end-line: 30

This is a program to read a set of `RULESET` files and write a manual to a set of HTML files of the same name.

OPTIONS
=======

The following options are accepted on the command line of the manual generator. They may not be combined as
with other tools. For example: ``freeciv21-manual -rw civ2civ3`` will not work. Instead you will need to enter
each option separately, such as, ``freevic21-manual -r civ2civ3 -w``.

``-d, --debug <LEVEL>``
    Set debug log level (fatal/critical/warning/info/debug). Default log level is ``info``.

``-F, --Fatal``
    Raise a signal on failed assertion. An assertion is a code calculation error. With this set, the client
    process will SEGFAULT instead of issuing a warning message to the terminal console.

``-l, --log <FILE>``
    Use FILE as logfile.

``-r, --ruleset <RULESET>``
    Make manual for RULESET.

``-h, --help``
    Display help on command line options.

``--help-all``
    Display help including Qt specific options.

``-v, --version``
    Display version information.

.. include:: freeciv21-server.rst
  :start-line: 148
