..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

freeciv21-ruleup
****************

SYNOPSIS
========

``freeciv21-ruleup`` [ -r|--ruleset `<RULESET>` ] [ -o|--output `<DIRECTORY>` ] [ -h|--help ] [ -v|--version ]

DESCRIPTION
===========

.. include:: freeciv21-server.rst
  :start-line: 17
  :end-line: 30

This command line utility allows a user to upgrade a ruleset designed for an older version of Freeciv21 to
work on a newer version. freeciv21-ruleup does not create well-formatted human-readable ruleset files, so some
hand editing will be needed to aid readability.

OPTIONS
=======

The following options are accepted on the command line of the ruleup tool. They may not be combined as
with other tools. For example: ``freeciv21-ruleup -ro ~/myrulset ~/myupgradedruleset`` will not work. Instead
you will need to enter each option separately, such as,
``freeciv21-ruleup -r ~/myruleset -o ~/myupgradedruleset``.

``-F, --Fatal``
    Raise a signal on failed assertion. An assertion is a code calculation error. With this set, the client
    process will SEGFAULT instead of issuing a warning message to the terminal console.

``-r, --ruleset``
    The path to the old ruleset that needs to be upgraded.

``-o, --output``
    The path to write the upgraded ruleset.

``-h, --help``
    Display help on command line options.

``--help-all``
    Display help including Qt specific options.

``-v, --version``
    Display version information.

.. include:: freeciv21-server.rst
  :start-line: 148


